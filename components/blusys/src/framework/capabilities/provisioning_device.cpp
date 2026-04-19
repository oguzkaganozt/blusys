#include "blusys/framework/capabilities/provisioning.hpp"
#include "blusys/framework/events/pending_events.hpp"
#include "blusys/framework/events/event.hpp"
#include "blusys/framework/events/event_queue.hpp"
#include "blusys/hal/log.h"
#include "blusys/framework/services/net.h"

#include <atomic>
#include <cstring>

static const char *TAG = "blusys_prov";

namespace blusys {

namespace {

constexpr std::uint32_t kPendingNone          = 0;
constexpr std::uint32_t kPendingStarted       = 1 << 0;
constexpr std::uint32_t kPendingCredentials   = 1 << 1;
constexpr std::uint32_t kPendingSuccess       = 1 << 2;
constexpr std::uint32_t kPendingFailed        = 1 << 3;
constexpr std::uint32_t kPendingResetComplete = 1 << 4;

}  // namespace

struct provisioning_capability::impl {
    blusys_wifi_prov_t           *prov          = nullptr;
    std::atomic<std::uint32_t>    pending_flags{kPendingNone};
    bool                          already_posted = false;
};

provisioning_capability::provisioning_capability(const provisioning_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

provisioning_capability::~provisioning_capability()
{
    delete impl_;
}

blusys_err_t provisioning_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.service_name == nullptr) {
        BLUSYS_LOGE(TAG, "service_name is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (cfg_.skip_if_provisioned && blusys_wifi_prov_is_provisioned()) {
        status_.is_provisioned = true;
        status_.capability_ready = true;
        BLUSYS_LOGI(TAG, "already provisioned — skipping");
        return BLUSYS_OK;
    }

    blusys_wifi_prov_config_t prov_cfg{};
    prov_cfg.transport    = static_cast<blusys_wifi_prov_transport_t>(cfg_.transport);
    prov_cfg.service_name = cfg_.service_name;
    prov_cfg.pop          = cfg_.pop;
    prov_cfg.service_key  = cfg_.service_key;
    prov_cfg.on_event     = [](blusys_wifi_prov_event_t ev,
                               const blusys_wifi_prov_credentials_t *creds,
                               void *ctx) {
        prov_event_handler(static_cast<int>(ev), creds, ctx);
    };
    prov_cfg.user_ctx     = this;

    blusys_err_t err = blusys_wifi_prov_open(&prov_cfg, &impl_->prov);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(TAG, "prov open failed: %d", static_cast<int>(err));
        return err;
    }

    blusys_wifi_prov_get_qr_payload(impl_->prov, status_.qr_payload,
                                    sizeof(status_.qr_payload));

    if (cfg_.auto_start) {
        err = blusys_wifi_prov_start(impl_->prov);
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
    if (status_.is_provisioned && status_.capability_ready && impl_->prov == nullptr) {
        if (!impl_->already_posted) {
            impl_->already_posted = true;
            post_event(provisioning_event::already_provisioned);
            post_event(provisioning_event::capability_ready);
        }
        return;
    }

    const std::uint32_t flags =
        detail::drain_pending_flags(impl_->pending_flags, kPendingNone);
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
        status_.capability_ready = true;
        post_event(provisioning_event::success);
        post_event(provisioning_event::capability_ready);
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
    if (impl_->prov != nullptr) {
        blusys_wifi_prov_stop(impl_->prov);
        blusys_wifi_prov_close(impl_->prov);
        impl_->prov = nullptr;
    }
    impl_->already_posted = false;
    status_ = {};
    rt_ = nullptr;
}

blusys_err_t provisioning_capability::request_reset()
{
    blusys_err_t err = blusys_wifi_prov_reset();
    if (err == BLUSYS_OK) {
        status_.is_provisioned = false;
        status_.provision_success = false;
        status_.credentials_received = false;
        status_.capability_ready = false;
        impl_->already_posted = false;
        impl_->pending_flags.fetch_or(kPendingResetComplete, std::memory_order_release);
        BLUSYS_LOGI(TAG, "provisioning credentials reset");
    }
    return err;
}

void provisioning_capability::prov_event_handler(int event, const void * /*creds*/,
                                                  void *user_ctx)
{
    auto *self = static_cast<provisioning_capability *>(user_ctx);
    switch (static_cast<blusys_wifi_prov_event_t>(event)) {
    case BLUSYS_WIFI_PROV_EVENT_STARTED:
        self->impl_->pending_flags.fetch_or(kPendingStarted, std::memory_order_release);
        break;
    case BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED:
        self->impl_->pending_flags.fetch_or(kPendingCredentials, std::memory_order_release);
        break;
    case BLUSYS_WIFI_PROV_EVENT_SUCCESS:
        self->impl_->pending_flags.fetch_or(kPendingSuccess, std::memory_order_release);
        break;
    case BLUSYS_WIFI_PROV_EVENT_FAILED:
        self->impl_->pending_flags.fetch_or(kPendingFailed, std::memory_order_release);
        break;
    }
}

}  // namespace blusys
