#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/engine/pending_events.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"
#include "blusys/framework/services/net.h"

#include <atomic>

static const char *TAG = "blusys_bt";

namespace blusys {

namespace {

constexpr std::uint32_t kPendingNone              = 0;
constexpr std::uint32_t kPendingGapReady          = 1 << 0;
constexpr std::uint32_t kPendingGattReady         = 1 << 1;
constexpr std::uint32_t kPendingClientConnected   = 1 << 2;
constexpr std::uint32_t kPendingClientDisconnected = 1 << 3;

}  // namespace

struct bluetooth_capability::impl {
    blusys_bluetooth_t           *bt           = nullptr;
    blusys_ble_gatt_t            *gatt         = nullptr;
    std::atomic<std::uint32_t>    pending_flags{kPendingNone};
};

bluetooth_capability::bluetooth_capability(const bluetooth_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

bluetooth_capability::~bluetooth_capability()
{
    delete impl_;
}

blusys_err_t bluetooth_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.device_name == nullptr) {
        BLUSYS_LOGE(TAG, "device_name is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (cfg_.services != nullptr && cfg_.svc_count > 0) {
        blusys_ble_gatt_config_t gatt_cfg{};
        gatt_cfg.device_name   = cfg_.device_name;
        gatt_cfg.services      = cfg_.services;
        gatt_cfg.svc_count     = cfg_.svc_count;
        gatt_cfg.conn_cb       = gatt_conn_handler;
        gatt_cfg.conn_user_ctx = this;

        blusys_err_t err = blusys_ble_gatt_open(&gatt_cfg, &impl_->gatt);
        if (err != BLUSYS_OK) {
            if (err == BLUSYS_ERR_BUSY) {
                BLUSYS_LOGE(TAG,
                            "ble gatt open failed: BLE controller busy (Wi-Fi provisioning, "
                            "another bluetooth_capability path, USB HID BLE, etc.)");
            } else {
                BLUSYS_LOGE(TAG, "ble gatt open failed: %d", static_cast<int>(err));
            }
            return err;
        }

        status_.gap_ready = true;
        status_.gatt_ready = true;
        status_.advertising = true;
        impl_->pending_flags.fetch_or(kPendingGapReady | kPendingGattReady,
                                      std::memory_order_release);
    } else {
        blusys_bluetooth_config_t bt_cfg{};
        bt_cfg.device_name = cfg_.device_name;

        blusys_err_t err = blusys_bluetooth_open(&bt_cfg, &impl_->bt);
        if (err != BLUSYS_OK) {
            if (err == BLUSYS_ERR_BUSY) {
                BLUSYS_LOGE(TAG,
                            "bluetooth open failed: BLE controller busy (Wi-Fi provisioning, "
                            "GATT path, USB HID BLE, etc.)");
            } else {
                BLUSYS_LOGE(TAG, "bluetooth open failed: %d", static_cast<int>(err));
            }
            return err;
        }

        status_.gap_ready = true;
        impl_->pending_flags.fetch_or(kPendingGapReady, std::memory_order_release);

        if (cfg_.auto_advertise) {
            err = blusys_bluetooth_advertise_start(impl_->bt);
            if (err == BLUSYS_OK) {
                status_.advertising = true;
            } else {
                BLUSYS_LOGW(TAG, "advertise start failed: %d", static_cast<int>(err));
            }
        }
    }

    return BLUSYS_OK;
}

void bluetooth_capability::poll(std::uint32_t /*now_ms*/)
{
    const std::uint32_t flags =
        detail::drain_pending_flags(impl_->pending_flags, kPendingNone);
    if (flags == kPendingNone) {
        return;
    }

    if (flags & kPendingGapReady) {
        post_event(bluetooth_event::gap_ready);
        if (status_.advertising) {
            post_event(bluetooth_event::advertising_started);
        }
    }
    if (flags & kPendingGattReady) {
        post_event(bluetooth_event::gatt_ready);
    }
    if (flags & kPendingClientConnected) {
        status_.client_connected = true;
        ++status_.connected_count;
        post_event(bluetooth_event::client_connected);
    }
    if (flags & kPendingClientDisconnected) {
        if (status_.connected_count > 0) {
            --status_.connected_count;
        }
        status_.client_connected = (status_.connected_count > 0);
        post_event(bluetooth_event::client_disconnected);
    }

    check_capability_ready();
}

void bluetooth_capability::stop()
{
    if (impl_->gatt != nullptr) {
        blusys_ble_gatt_close(impl_->gatt);
        impl_->gatt = nullptr;
    }
    if (impl_->bt != nullptr) {
        blusys_bluetooth_advertise_stop(impl_->bt);
        blusys_bluetooth_close(impl_->bt);
        impl_->bt = nullptr;
    }
    status_ = {};
    rt_ = nullptr;
}

void bluetooth_capability::gatt_conn_handler(std::uint16_t conn_handle,
                                             bool connected,
                                             void *user_ctx)
{
    auto *self = static_cast<bluetooth_capability *>(user_ctx);
    if (connected) {
        self->impl_->pending_flags.fetch_or(kPendingClientConnected,
                                            std::memory_order_release);
    } else {
        self->impl_->pending_flags.fetch_or(kPendingClientDisconnected,
                                            std::memory_order_release);
    }
    if (self->cfg_.conn_cb != nullptr) {
        self->cfg_.conn_cb(conn_handle, connected, self->cfg_.conn_user_ctx);
    }
}

void bluetooth_capability::check_capability_ready()
{
    if (status_.capability_ready) {
        return;
    }

    bool ready = status_.gap_ready;
    if (cfg_.services != nullptr && !status_.gatt_ready) {
        ready = false;
    }

    if (ready) {
        status_.capability_ready = true;
        post_event(bluetooth_event::capability_ready);
        BLUSYS_LOGI(TAG, "bluetooth capability ready");
    }
}

}  // namespace blusys
