#ifdef ESP_PLATFORM

#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

#include <cstring>

static const char *TAG = "blusys_prov";

namespace blusys::app {

provisioning_capability::provisioning_capability(const provisioning_config &cfg)
    : cfg_(cfg)
{
}

blusys_err_t provisioning_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.service_name == nullptr) {
        BLUSYS_LOGE(TAG, "service_name is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    // Check if already provisioned.
    if (cfg_.skip_if_provisioned && blusys_wifi_prov_is_provisioned()) {
        status_.is_provisioned = true;
        status_.bundle_ready = true;
        BLUSYS_LOGI(TAG, "already provisioned — skipping");
        // Events posted on first poll via pending flags would require
        // a different mechanism. Since we're in start(), post directly
        // — the reducer controller isn't processing events yet at this
        // point, so we defer to poll().
        return BLUSYS_OK;
    }

    blusys_wifi_prov_config_t prov_cfg{};
    prov_cfg.transport    = cfg_.transport;
    prov_cfg.service_name = cfg_.service_name;
    prov_cfg.pop          = cfg_.pop;
    prov_cfg.service_key  = cfg_.service_key;
    prov_cfg.on_event     = prov_event_handler;
    prov_cfg.user_ctx     = this;

    blusys_err_t err = blusys_wifi_prov_open(&prov_cfg, &prov_);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(TAG, "prov open failed: %d", static_cast<int>(err));
        return err;
    }

    // Retrieve QR payload.
    blusys_wifi_prov_get_qr_payload(prov_, status_.qr_payload,
                                    sizeof(status_.qr_payload));

    if (cfg_.auto_start) {
        err = blusys_wifi_prov_start(prov_);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(TAG, "prov start failed: %d", static_cast<int>(err));
            return err;
        }
        status_.service_running = true;
    }

    return BLUSYS_OK;
}

void provisioning_capability::poll(std::uint32_t /*now_ms*/)
{
    // Handle the "already provisioned" case — post events on first poll.
    if (status_.is_provisioned && status_.bundle_ready && prov_ == nullptr) {
        if (!already_posted_) {
            already_posted_ = true;
            post_event(provisioning_event::already_provisioned);
            post_event(provisioning_event::bundle_ready);
        }
        return;
    }

    const std::uint32_t flags =
        pending_flags_.exchange(kPendingNone, std::memory_order_acquire);
    if (flags == kPendingNone) {
        return;
    }

    if (flags & kPendingStarted) {
        post_event(provisioning_event::started);
    }
    if (flags & kPendingCredentials) {
        status_.credentials_received = true;
        post_event(provisioning_event::credentials_received);
    }
    if (flags & kPendingSuccess) {
        status_.provision_success = true;
        status_.is_provisioned = true;
        status_.bundle_ready = true;
        post_event(provisioning_event::success);
        post_event(provisioning_event::bundle_ready);
    }
    if (flags & kPendingFailed) {
        status_.provision_failed = true;
        post_event(provisioning_event::failed);
    }
    if (flags & kPendingResetComplete) {
        post_event(provisioning_event::reset_complete);
    }
}

void provisioning_capability::stop()
{
    if (prov_ != nullptr) {
        blusys_wifi_prov_stop(prov_);
        blusys_wifi_prov_close(prov_);
        prov_ = nullptr;
    }
    status_ = {};
    already_posted_ = false;
    rt_ = nullptr;
}

blusys_err_t provisioning_capability::request_reset()
{
    blusys_err_t err = blusys_wifi_prov_reset();
    if (err == BLUSYS_OK) {
        status_.is_provisioned = false;
        status_.provision_success = false;
        status_.credentials_received = false;
        status_.bundle_ready = false;
        already_posted_ = false;
        pending_flags_.fetch_or(kPendingResetComplete, std::memory_order_release);
        BLUSYS_LOGI(TAG, "provisioning credentials reset");
    }
    return err;
}

void provisioning_capability::prov_event_handler(blusys_wifi_prov_event_t event,
                                                 const blusys_wifi_prov_credentials_t * /*creds*/,
                                                 void *user_ctx)
{
    auto *self = static_cast<provisioning_capability *>(user_ctx);

    switch (event) {
    case BLUSYS_WIFI_PROV_EVENT_STARTED:
        self->pending_flags_.fetch_or(kPendingStarted, std::memory_order_release);
        break;
    case BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED:
        self->pending_flags_.fetch_or(kPendingCredentials, std::memory_order_release);
        break;
    case BLUSYS_WIFI_PROV_EVENT_SUCCESS:
        self->pending_flags_.fetch_or(kPendingSuccess, std::memory_order_release);
        break;
    case BLUSYS_WIFI_PROV_EVENT_FAILED:
        self->pending_flags_.fetch_or(kPendingFailed, std::memory_order_release);
        break;
    }
}

void provisioning_capability::post_event(provisioning_event ev)
{
    if (rt_ != nullptr) {
        rt_->post_event(blusys::framework::make_integration_event(
            static_cast<std::uint32_t>(ev)));
    }
}

}  // namespace blusys::app

#endif  // ESP_PLATFORM
