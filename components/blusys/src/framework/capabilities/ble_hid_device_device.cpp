#include "blusys/framework/capabilities/ble_hid_device.hpp"
#include "blusys/framework/engine/pending_events.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"
#include "blusys/services/connectivity/ble_hid_device.h"

#include <atomic>

static const char *TAG = "blusys_ble_hid";

namespace blusys {

namespace {

constexpr std::uint32_t kPendingNone               = 0;
constexpr std::uint32_t kPendingAdvertisingStarted = 1 << 0;
constexpr std::uint32_t kPendingClientConnected    = 1 << 1;
constexpr std::uint32_t kPendingClientDisconnected = 1 << 2;

}  // namespace

struct ble_hid_device_capability::impl {
    blusys_ble_hid_device_t       *hid          = nullptr;
    std::atomic<std::uint32_t>     pending_flags{kPendingNone};
};

ble_hid_device_capability::ble_hid_device_capability(const ble_hid_device_config &cfg)
    : cfg_(cfg), impl_(new impl{})
{
}

ble_hid_device_capability::~ble_hid_device_capability()
{
    delete impl_;
}

blusys_err_t ble_hid_device_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.device_name == nullptr) {
        BLUSYS_LOGE(TAG, "device_name is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_ble_hid_device_config_t hid_cfg{};
    hid_cfg.device_name          = cfg_.device_name;
    hid_cfg.vendor_id            = cfg_.vendor_id;
    hid_cfg.product_id           = cfg_.product_id;
    hid_cfg.version              = cfg_.version;
    hid_cfg.initial_battery_pct  = cfg_.initial_battery_pct;
    hid_cfg.conn_cb              = conn_handler;
    hid_cfg.conn_user_ctx        = this;

    blusys_err_t err = blusys_ble_hid_device_open(&hid_cfg, &impl_->hid);
    if (err != BLUSYS_OK) {
        if (err == BLUSYS_ERR_BUSY) {
            BLUSYS_LOGE(TAG,
                        "ble hid device open failed: BLE controller busy "
                        "(Wi-Fi provisioning, bluetooth_capability, ble_gatt, "
                        "or USB HID BLE already active)");
        } else {
            BLUSYS_LOGE(TAG, "ble hid device open failed: %d", static_cast<int>(err));
        }
        return err;
    }

    status_.advertising = true;
    impl_->pending_flags.fetch_or(kPendingAdvertisingStarted, std::memory_order_release);
    return BLUSYS_OK;
}

void ble_hid_device_capability::poll(std::uint32_t /*now_ms*/)
{
    const std::uint32_t flags =
        detail::drain_pending_flags(impl_->pending_flags, kPendingNone);
    if (flags == kPendingNone) {
        return;
    }

    if (flags & kPendingAdvertisingStarted) {
        post_event(ble_hid_device_event::advertising_started);
    }
    if (flags & kPendingClientConnected) {
        status_.client_connected = true;
        post_event(ble_hid_device_event::client_connected);
    }
    if (flags & kPendingClientDisconnected) {
        status_.client_connected = false;
        post_event(ble_hid_device_event::client_disconnected);
    }

    check_capability_ready();
}

void ble_hid_device_capability::stop()
{
    if (impl_->hid != nullptr) {
        blusys_ble_hid_device_close(impl_->hid);
        impl_->hid = nullptr;
    }
    status_ = {};
    rt_ = nullptr;
}

void ble_hid_device_capability::conn_handler(bool connected, void *user_ctx)
{
    auto *self = static_cast<ble_hid_device_capability *>(user_ctx);
    if (connected) {
        self->impl_->pending_flags.fetch_or(kPendingClientConnected,
                                            std::memory_order_release);
    } else {
        self->impl_->pending_flags.fetch_or(kPendingClientDisconnected,
                                            std::memory_order_release);
    }
}

void ble_hid_device_capability::check_capability_ready()
{
    if (status_.capability_ready) {
        return;
    }
    if (status_.advertising) {
        status_.capability_ready = true;
        post_event(ble_hid_device_event::capability_ready);
        BLUSYS_LOGI(TAG, "ble_hid_device capability ready ('%s')", cfg_.device_name);
    }
}

blusys_err_t ble_hid_device_capability::send_consumer(std::uint16_t usage, bool pressed)
{
    if (impl_->hid == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_ble_hid_device_send_consumer(impl_->hid, usage, pressed);
}

blusys_err_t ble_hid_device_capability::set_battery(std::uint8_t percent)
{
    if (impl_->hid == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return blusys_ble_hid_device_set_battery(impl_->hid, percent);
}

bool ble_hid_device_capability::is_connected() const
{
    return impl_->hid != nullptr && blusys_ble_hid_device_is_connected(impl_->hid);
}

bool ble_hid_device_capability::is_ready() const
{
    return impl_->hid != nullptr && blusys_ble_hid_device_is_ready(impl_->hid);
}

}  // namespace blusys
