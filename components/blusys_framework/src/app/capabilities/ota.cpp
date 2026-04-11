#ifdef ESP_PLATFORM

#include "blusys/app/capabilities/ota.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

#include "esp_ota_ops.h"

static const char *TAG = "blusys_ota";

namespace blusys::app {

ota_capability::ota_capability(const ota_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t ota_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;

    // Check rollback state on boot.
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            status_.rollback_pending = true;
            if (cfg_.auto_mark_valid) {
                blusys_err_t err = blusys_ota_mark_valid();
                if (err == BLUSYS_OK) {
                    status_.rollback_pending = false;
                    status_.marked_valid = true;
                    pending_flags_.fetch_or(kPendingMarkedValid, std::memory_order_release);
                    BLUSYS_LOGI(TAG, "firmware marked valid (auto)");
                } else {
                    pending_flags_.fetch_or(kPendingRollback, std::memory_order_release);
                    BLUSYS_LOGW(TAG, "mark valid failed: %d", static_cast<int>(err));
                }
            } else {
                pending_flags_.fetch_or(kPendingRollback, std::memory_order_release);
                BLUSYS_LOGI(TAG, "rollback pending — waiting for manual mark_valid");
            }
        }
    }

    // Defer capability_ready to first poll so reducer receives it through
    // the normal event flow (not during init).
    pending_flags_.fetch_or(kPendingCapabilityReady, std::memory_order_release);
    return BLUSYS_OK;
}

void ota_capability::poll(std::uint32_t /*now_ms*/)
{
    const std::uint32_t flags =
        pending_flags_.exchange(kPendingNone, std::memory_order_acquire);
    if (flags == kPendingNone) {
        return;
    }

    if (flags & kPendingRollback) {
        post_event(ota_event::rollback_pending);
    }
    if (flags & kPendingMarkedValid) {
        post_event(ota_event::marked_valid);
    }
    if (flags & kPendingDownloadStarted) {
        post_event(ota_event::download_started);
    }
    if (flags & kPendingApplyComplete) {
        status_.downloading = false;
        status_.download_complete = true;
        status_.apply_complete = true;
        post_event(ota_event::download_complete);
        post_event(ota_event::apply_complete);
    }
    if (flags & kPendingApplyFailed) {
        status_.downloading = false;
        post_event(ota_event::download_failed);
        post_event(ota_event::apply_failed);
    }
    if (flags & kPendingCapabilityReady) {
        status_.capability_ready = true;
        post_event(ota_event::capability_ready);
    }
}

void ota_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

blusys_err_t ota_capability::request_update()
{
    if (cfg_.firmware_url == nullptr) {
        BLUSYS_LOGE(TAG, "firmware_url is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    status_.downloading = true;
    pending_flags_.fetch_or(kPendingDownloadStarted, std::memory_order_release);

    blusys_ota_config_t ota_cfg{};
    ota_cfg.url        = cfg_.firmware_url;
    ota_cfg.cert_pem   = cfg_.cert_pem;
    ota_cfg.timeout_ms = cfg_.timeout_ms;

    blusys_ota_t *handle = nullptr;
    blusys_err_t err = blusys_ota_open(&ota_cfg, &handle);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(TAG, "ota open failed: %d", static_cast<int>(err));
        pending_flags_.fetch_or(kPendingApplyFailed, std::memory_order_release);
        return err;
    }

    err = blusys_ota_perform(handle);
    blusys_ota_close(handle);

    if (err == BLUSYS_OK) {
        BLUSYS_LOGI(TAG, "OTA download + flash complete");
        pending_flags_.fetch_or(kPendingApplyComplete, std::memory_order_release);
    } else {
        BLUSYS_LOGE(TAG, "OTA perform failed: %d", static_cast<int>(err));
        pending_flags_.fetch_or(kPendingApplyFailed, std::memory_order_release);
    }

    return err;
}

void ota_capability::post_event(ota_event ev)
{
    if (rt_ != nullptr) {
        rt_->post_event(blusys::framework::make_integration_event(
            static_cast<std::uint32_t>(ev)));
    }
}

}  // namespace blusys::app

#endif  // ESP_PLATFORM
