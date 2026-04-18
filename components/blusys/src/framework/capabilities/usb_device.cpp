#include "blusys/framework/capabilities/usb.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"
#include "blusys/hal/usb_device.h"
#include "blusys/hal/usb_host.h"
#include "blusys/hal/target.h"

#include <atomic>

static const char *TAG = "blusys_usb_cap";

namespace blusys {

namespace {

constexpr std::uint32_t kPendingNone              = 0;
constexpr std::uint32_t kPendingDeviceConnected   = 1 << 0;
constexpr std::uint32_t kPendingDeviceDisconnected = 1 << 1;
constexpr std::uint32_t kPendingDeviceSuspended   = 1 << 2;
constexpr std::uint32_t kPendingDeviceResumed     = 1 << 3;
constexpr std::uint32_t kPendingHostAttached      = 1 << 4;
constexpr std::uint32_t kPendingHostDetached      = 1 << 5;

constexpr bool mask_has_single_bit(std::uint8_t mask)
{
    return mask != 0 && (mask & (mask - 1)) == 0;
}

blusys_usb_device_class_t get_usb_device_class(const usb_config &cfg)
{
    if (cfg.class_mask & static_cast<std::uint8_t>(usb_class::hid)) {
        return BLUSYS_USB_DEVICE_CLASS_HID;
    }
    if (cfg.class_mask & static_cast<std::uint8_t>(usb_class::msc)) {
        return BLUSYS_USB_DEVICE_CLASS_MSC;
    }
    return BLUSYS_USB_DEVICE_CLASS_CDC;
}

}  // namespace

struct usb_capability::impl {
    blusys_usb_device_t           *device        = nullptr;
    blusys_usb_host_t             *host          = nullptr;
    std::atomic<std::uint32_t>     pending_flags{kPendingNone};
};

usb_capability::usb_capability(const usb_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

usb_capability::~usb_capability()
{
    delete impl_;
}

blusys_err_t usb_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.role == usb_role::device) {
        if (!blusys_target_supports(BLUSYS_FEATURE_USB_DEVICE)) {
            return BLUSYS_ERR_NOT_SUPPORTED;
        }
        if (!mask_has_single_bit(cfg_.class_mask)) {
            BLUSYS_LOGE(TAG, "usb device currently requires exactly one class bit");
            return BLUSYS_ERR_INVALID_ARG;
        }

        blusys_usb_device_config_t device_cfg{};
        device_cfg.vid          = cfg_.vid;
        device_cfg.pid          = cfg_.pid;
        device_cfg.manufacturer = cfg_.manufacturer;
        device_cfg.product      = cfg_.product;
        device_cfg.serial       = cfg_.serial;
        device_cfg.device_class = get_usb_device_class(cfg_);
        device_cfg.event_cb     = [](blusys_usb_device_t *dev, blusys_usb_device_event_t ev,
                                     void *ctx) {
            device_event_handler(dev, static_cast<int>(ev), ctx);
        };
        device_cfg.event_user_ctx = this;

        blusys_err_t err = blusys_usb_device_open(&device_cfg, &impl_->device);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(TAG, "usb_device open failed: %d", static_cast<int>(err));
            return err;
        }

        status_.role_active = true;
        status_.device_ready = true;
        post_event(usb_event::device_ready);
    } else {
        if (!blusys_target_supports(BLUSYS_FEATURE_USB_HOST)) {
            return BLUSYS_ERR_NOT_SUPPORTED;
        }

        blusys_usb_host_config_t host_cfg{};
        host_cfg.event_cb = [](blusys_usb_host_t *h, blusys_usb_host_event_t ev,
                                const blusys_usb_host_device_info_t *info, void *ctx) {
            host_event_handler(h, static_cast<int>(ev), info, ctx);
        };
        host_cfg.event_user_ctx = this;

        blusys_err_t err = blusys_usb_host_open(&host_cfg, &impl_->host);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(TAG, "usb_host open failed: %d", static_cast<int>(err));
            return err;
        }

        status_.role_active = true;
        status_.host_ready = true;
        post_event(usb_event::host_ready);
    }

    check_capability_ready();
    return BLUSYS_OK;
}

void usb_capability::poll(std::uint32_t /*now_ms*/)
{
    const std::uint32_t flags =
        impl_->pending_flags.exchange(kPendingNone, std::memory_order_acq_rel);

    if (flags & kPendingDeviceConnected) {
        status_.device_connected = true;
        post_event(usb_event::device_connected);
    }
    if (flags & kPendingDeviceDisconnected) {
        status_.device_connected = false;
        post_event(usb_event::device_disconnected);
    }
    if (flags & kPendingDeviceSuspended) {
        post_event(usb_event::device_suspended);
    }
    if (flags & kPendingDeviceResumed) {
        post_event(usb_event::device_resumed);
    }
    if (flags & kPendingHostAttached) {
        status_.host_attached = true;
        post_event(usb_event::host_attached);
    }
    if (flags & kPendingHostDetached) {
        status_.host_attached = false;
        post_event(usb_event::host_detached);
    }
}

void usb_capability::stop()
{
    if (impl_->host != nullptr) {
        blusys_usb_host_close(impl_->host);
        impl_->host = nullptr;
    }
    if (impl_->device != nullptr) {
        blusys_usb_device_close(impl_->device);
        impl_->device = nullptr;
    }

    status_ = {};
    rt_ = nullptr;
}

void usb_capability::device_event_handler(blusys_usb_device_t * /*dev*/, int event,
                                          void *user_ctx)
{
    auto *self = static_cast<usb_capability *>(user_ctx);
    switch (static_cast<blusys_usb_device_event_t>(event)) {
    case BLUSYS_USB_DEVICE_EVENT_CONNECTED:
        self->impl_->pending_flags.fetch_or(kPendingDeviceConnected, std::memory_order_release);
        break;
    case BLUSYS_USB_DEVICE_EVENT_DISCONNECTED:
        self->impl_->pending_flags.fetch_or(kPendingDeviceDisconnected, std::memory_order_release);
        break;
    case BLUSYS_USB_DEVICE_EVENT_SUSPENDED:
        self->impl_->pending_flags.fetch_or(kPendingDeviceSuspended, std::memory_order_release);
        break;
    case BLUSYS_USB_DEVICE_EVENT_RESUMED:
        self->impl_->pending_flags.fetch_or(kPendingDeviceResumed, std::memory_order_release);
        break;
    }
}

void usb_capability::host_event_handler(blusys_usb_host_t * /*host*/, int event,
                                        const void * /*device_info*/, void *user_ctx)
{
    auto *self = static_cast<usb_capability *>(user_ctx);
    switch (static_cast<blusys_usb_host_event_t>(event)) {
    case BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED:
        self->impl_->pending_flags.fetch_or(kPendingHostAttached, std::memory_order_release);
        break;
    case BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED:
        self->impl_->pending_flags.fetch_or(kPendingHostDetached, std::memory_order_release);
        break;
    }
}

void usb_capability::check_capability_ready()
{
    if (status_.capability_ready) {
        return;
    }

    if ((cfg_.role == usb_role::device && status_.device_ready) ||
        (cfg_.role == usb_role::host && status_.host_ready)) {
        status_.capability_ready = true;
        post_event(usb_event::capability_ready);
        BLUSYS_LOGI(TAG, "usb capability ready");
    }
}

}  // namespace blusys
