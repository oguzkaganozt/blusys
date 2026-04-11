#pragma once

#include "blusys/app/capability.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>

#ifdef ESP_PLATFORM
#include "blusys/connectivity/wifi_prov.h"
#endif

namespace blusys::framework { class runtime; }

namespace blusys::app {

// ---- shared types (always available) ----

enum class provisioning_event : std::uint32_t {
    started              = 0x0700,
    credentials_received = 0x0701,
    success              = 0x0702,
    failed               = 0x0703,
    already_provisioned  = 0x0710,
    reset_complete       = 0x0711,
    capability_ready         = 0x07FF,
};

struct provisioning_status {
    bool is_provisioned       = false;
    bool service_running      = false;
    bool credentials_received = false;
    bool provision_success    = false;
    bool provision_failed     = false;
    bool capability_ready         = false;
    char qr_payload[256]      = {};
};

// ---- device implementation ----

#ifdef ESP_PLATFORM

struct provisioning_config {
    blusys_wifi_prov_transport_t transport = BLUSYS_WIFI_PROV_TRANSPORT_BLE;
    const char *service_name     = nullptr;
    const char *pop              = nullptr;
    const char *service_key      = nullptr;
    bool auto_start              = true;
    bool skip_if_provisioned     = true;
};

class provisioning_capability final : public capability_base {
public:
    explicit provisioning_capability(const provisioning_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::provisioning; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const provisioning_status &status() const { return status_; }

    // Erase stored credentials and restart provisioning.
    blusys_err_t request_reset();

private:
    static constexpr std::uint32_t kPendingNone         = 0;
    static constexpr std::uint32_t kPendingStarted      = 1 << 0;
    static constexpr std::uint32_t kPendingCredentials   = 1 << 1;
    static constexpr std::uint32_t kPendingSuccess       = 1 << 2;
    static constexpr std::uint32_t kPendingFailed        = 1 << 3;
    static constexpr std::uint32_t kPendingResetComplete  = 1 << 4;

    static void prov_event_handler(blusys_wifi_prov_event_t event,
                                   const blusys_wifi_prov_credentials_t *creds,
                                   void *user_ctx);

    void post_event(provisioning_event ev);

    provisioning_config cfg_;
    provisioning_status status_{};
    blusys::framework::runtime *rt_ = nullptr;
    blusys_wifi_prov_t *prov_ = nullptr;
    std::atomic<std::uint32_t> pending_flags_{kPendingNone};
    bool already_posted_ = false;
};

#else  // host stub

struct provisioning_config {
    int transport           = 0;
    const char *service_name = nullptr;
    const char *pop          = nullptr;
    const char *service_key  = nullptr;
    bool auto_start          = true;
    bool skip_if_provisioned = true;
};

class provisioning_capability final : public capability_base {
public:
    explicit provisioning_capability(const provisioning_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::provisioning; }

    blusys_err_t start(blusys::framework::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const provisioning_status &status() const { return status_; }

    blusys_err_t request_reset();

private:
    void post_event(provisioning_event ev);

    provisioning_config cfg_;
    provisioning_status status_{};
    blusys::framework::runtime *rt_ = nullptr;
    bool first_poll_ = true;
};

#endif  // ESP_PLATFORM

}  // namespace blusys::app
