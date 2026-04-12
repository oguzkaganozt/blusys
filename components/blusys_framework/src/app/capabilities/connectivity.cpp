#ifdef ESP_PLATFORM

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/detail/pending_events.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

#include "nvs_flash.h"

static const char *TAG = "blusys_conn";

namespace blusys::app {

connectivity_capability::connectivity_capability(const connectivity_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t connectivity_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;

    // NVS is required by the Wi-Fi driver.
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_err = nvs_flash_init();
    }
    if (nvs_err != ESP_OK) {
        BLUSYS_LOGE(TAG, "NVS init failed: %d", nvs_err);
        return BLUSYS_ERR_INTERNAL;
    }

    if (cfg_.wifi_ssid == nullptr) {
        BLUSYS_LOGE(TAG, "wifi_ssid is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_wifi_sta_config_t wifi_cfg{};
    wifi_cfg.ssid              = cfg_.wifi_ssid;
    wifi_cfg.password          = cfg_.wifi_password;
    wifi_cfg.auto_reconnect    = cfg_.auto_reconnect;
    wifi_cfg.reconnect_delay_ms = cfg_.reconnect_delay_ms;
    wifi_cfg.max_retries       = cfg_.max_retries;
    wifi_cfg.on_event          = wifi_event_handler;
    wifi_cfg.user_ctx          = this;

    blusys_err_t err = blusys_wifi_open(&wifi_cfg, &wifi_);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(TAG, "wifi open failed: %d", static_cast<int>(err));
        return err;
    }

    err = blusys_wifi_connect(wifi_, cfg_.connect_timeout_ms);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(TAG, "wifi connect failed: %d", static_cast<int>(err));
        // Non-fatal if auto_reconnect is enabled — the capability will retry.
        if (!cfg_.auto_reconnect) {
            blusys_wifi_close(wifi_);
            wifi_ = nullptr;
            return err;
        }
    }

    return BLUSYS_OK;
}

void connectivity_capability::poll(std::uint32_t now_ms)
{
    const std::uint32_t flags =
        detail::drain_pending_flags(pending_flags_, kPendingNone);

    if (flags & kPendingManualConnecting) {
        status_.wifi_connecting = true;
        post_event(connectivity_event::wifi_connecting);
    }

    if (flags & kPendingDriverStarted) {
        post_event(connectivity_event::wifi_started);
    }
    if (flags & kPendingDriverConnecting) {
        status_.wifi_connecting = true;
        post_event(connectivity_event::wifi_connecting);
    }

    if (flags & kPendingDisconnected) {
        status_.wifi_connected = false;
        status_.has_ip         = false;
        status_.wifi_connecting = false;
        status_.wifi_reconnecting = false;
        status_.capability_ready   = false;
        post_event(connectivity_event::wifi_disconnected);
    }

    if (flags & kPendingReconnecting) {
        status_.wifi_reconnecting = true;
        status_.wifi_connecting = false;
        post_event(connectivity_event::wifi_reconnecting);
    }

    if (flags & kPendingConnected) {
        status_.wifi_connected = true;
        status_.wifi_connecting = false;
        status_.wifi_reconnecting = false;
        post_event(connectivity_event::wifi_connected);
    }

    if (flags & kPendingGotIp) {
        status_.has_ip = true;
        status_.wifi_connecting = false;
        status_.wifi_reconnecting = false;
        blusys_wifi_get_ip_info(wifi_, &status_.ip_info);
        post_event(connectivity_event::wifi_got_ip);

        if (!dependent_services_started_) {
            start_dependent_services(now_ms);
        }

        check_capability_ready();
    }

    if (sntp_ != nullptr && !status_.time_synced) {
        if (blusys_sntp_is_synced(sntp_)) {
            status_.time_synced = true;
            sntp_timeout_reported_ = false;
            post_event(connectivity_event::time_synced);
            BLUSYS_LOGI(TAG, "time synced");
            check_capability_ready();
        } else if (!sntp_timeout_reported_ && cfg_.sntp_timeout_ms > 0 &&
                   now_ms - sntp_sync_started_ms_ >=
                       static_cast<std::uint32_t>(cfg_.sntp_timeout_ms)) {
            sntp_timeout_reported_ = true;
            post_event(connectivity_event::time_sync_failed);
            BLUSYS_LOGW(TAG, "time sync timed out after %d ms", cfg_.sntp_timeout_ms);
        }
    }
}

void connectivity_capability::stop()
{
    if (ctrl_ != nullptr) {
        blusys_local_ctrl_close(ctrl_);
        ctrl_ = nullptr;
    }
    if (mdns_ != nullptr) {
        blusys_mdns_close(mdns_);
        mdns_ = nullptr;
    }
    if (sntp_ != nullptr) {
        blusys_sntp_close(sntp_);
        sntp_ = nullptr;
    }
    if (wifi_ != nullptr) {
        blusys_wifi_disconnect(wifi_);
        blusys_wifi_close(wifi_);
        wifi_ = nullptr;
    }

    status_ = {};
    dependent_services_started_ = false;
    sntp_sync_started_ms_ = 0;
    sntp_timeout_reported_ = false;
    rt_ = nullptr;
}

void connectivity_capability::wifi_event_handler(blusys_wifi_t * /*wifi*/,
                                                 blusys_wifi_event_t event,
                                                 const blusys_wifi_event_info_t * /*info*/,
                                                 void *user_ctx)
{
    auto *self = static_cast<connectivity_capability *>(user_ctx);

    switch (event) {
    case BLUSYS_WIFI_EVENT_STARTED:
        self->pending_flags_.fetch_or(kPendingDriverStarted, std::memory_order_release);
        break;
    case BLUSYS_WIFI_EVENT_CONNECTING:
        self->pending_flags_.fetch_or(kPendingDriverConnecting, std::memory_order_release);
        break;
    case BLUSYS_WIFI_EVENT_CONNECTED:
        self->pending_flags_.fetch_or(kPendingConnected, std::memory_order_release);
        break;
    case BLUSYS_WIFI_EVENT_GOT_IP:
        self->pending_flags_.fetch_or(kPendingGotIp, std::memory_order_release);
        break;
    case BLUSYS_WIFI_EVENT_DISCONNECTED:
        self->pending_flags_.fetch_or(kPendingDisconnected, std::memory_order_release);
        break;
    case BLUSYS_WIFI_EVENT_RECONNECTING:
        self->pending_flags_.fetch_or(kPendingReconnecting, std::memory_order_release);
        break;
    default:
        break;
    }
}

void connectivity_capability::start_dependent_services(std::uint32_t now_ms)
{
    dependent_services_started_ = true;

    // SNTP
    if (cfg_.sntp_server != nullptr) {
        blusys_sntp_config_t sntp_cfg{};
        sntp_cfg.server      = cfg_.sntp_server;
        sntp_cfg.server2     = cfg_.sntp_server2;
        sntp_cfg.smooth_sync = cfg_.sntp_smooth_sync;

        blusys_err_t err = blusys_sntp_open(&sntp_cfg, &sntp_);
        if (err == BLUSYS_OK) {
            sntp_sync_started_ms_ = now_ms;
            sntp_timeout_reported_ = false;
            if (blusys_sntp_is_synced(sntp_)) {
                status_.time_synced = true;
                post_event(connectivity_event::time_synced);
                BLUSYS_LOGI(TAG, "time synced");
            }
        } else {
            BLUSYS_LOGW(TAG, "sntp open failed: %d", static_cast<int>(err));
        }
    }

    // mDNS
    if (cfg_.mdns_hostname != nullptr) {
        blusys_mdns_config_t mdns_cfg{};
        mdns_cfg.hostname      = cfg_.mdns_hostname;
        mdns_cfg.instance_name = cfg_.mdns_instance_name;

        blusys_err_t err = blusys_mdns_open(&mdns_cfg, &mdns_);
        if (err == BLUSYS_OK) {
            status_.mdns_running = true;
            post_event(connectivity_event::mdns_ready);
            BLUSYS_LOGI(TAG, "mDNS ready: %s.local", cfg_.mdns_hostname);
        } else {
            BLUSYS_LOGW(TAG, "mdns open failed: %d", static_cast<int>(err));
        }
    }

    // Local control
    if (cfg_.local_ctrl_device_name != nullptr) {
        blusys_local_ctrl_config_t ctrl_cfg{};
        ctrl_cfg.device_name    = cfg_.local_ctrl_device_name;
        ctrl_cfg.http_port      = cfg_.local_ctrl_port;
        ctrl_cfg.actions        = cfg_.local_ctrl_actions;
        ctrl_cfg.action_count   = cfg_.local_ctrl_action_count;
        ctrl_cfg.status_cb      = cfg_.local_ctrl_status_cb;
        ctrl_cfg.user_ctx       = cfg_.local_ctrl_user_ctx;

        blusys_err_t err = blusys_local_ctrl_open(&ctrl_cfg, &ctrl_);
        if (err == BLUSYS_OK) {
            status_.local_ctrl_running = true;
            post_event(connectivity_event::local_ctrl_ready);
            BLUSYS_LOGI(TAG, "local control ready on port %u",
                         static_cast<unsigned>(cfg_.local_ctrl_port));
        } else {
            BLUSYS_LOGW(TAG, "local_ctrl open failed: %d", static_cast<int>(err));
        }
    }
}

blusys_err_t connectivity_capability::request_reconnect()
{
    if (wifi_ == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    BLUSYS_LOGI(TAG, "manual reconnect requested");
    status_.wifi_connecting = true;
    status_.wifi_reconnecting = false;
    pending_flags_.fetch_or(kPendingManualConnecting, std::memory_order_release);
    blusys_wifi_disconnect(wifi_);
    return blusys_wifi_connect(wifi_, cfg_.connect_timeout_ms);
}

void connectivity_capability::check_capability_ready()
{
    if (status_.capability_ready) {
        return;
    }

    bool ready = status_.has_ip;

    if (cfg_.sntp_server != nullptr && !status_.time_synced) {
        ready = false;
    }
    if (cfg_.mdns_hostname != nullptr && !status_.mdns_running) {
        ready = false;
    }
    if (cfg_.local_ctrl_device_name != nullptr && !status_.local_ctrl_running) {
        ready = false;
    }

    if (ready) {
        status_.capability_ready = true;
        post_event(connectivity_event::capability_ready);
        BLUSYS_LOGI(TAG, "connectivity capability ready (ip=%s)", status_.ip_info.ip);
    }
}

}  // namespace blusys::app

#endif  // ESP_PLATFORM
