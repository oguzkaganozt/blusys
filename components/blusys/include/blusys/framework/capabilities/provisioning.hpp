#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include <cstdint>

// Forward declaration for device-side Wi-Fi provisioning handle.
struct blusys_wifi_prov;
typedef struct blusys_wifi_prov blusys_wifi_prov_t;

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class provisioning_event : std::uint32_t {
    started              = 0x0700,
    credentials_received = 0x0701,
    success              = 0x0702,
    failed               = 0x0703,
    already_provisioned  = 0x0710,
    reset_complete       = 0x0711,
    capability_ready     = 0x07FF,
};

struct provisioning_status : capability_status_base {
    bool is_provisioned       = false;
    bool service_running      = false;
    bool credentials_received = false;
    bool provision_success    = false;
    bool provision_failed     = false;
    char qr_payload[256]      = {};
};

// ---- configuration ----

struct provisioning_config {
    // On device, cast to blusys_wifi_prov_transport_t.
    // 0 = BLE, 1 = SoftAP (matches BLUSYS_WIFI_PROV_TRANSPORT_BLE/SOFTAP).
    int         transport           = 0;
    const char *service_name        = nullptr;
    const char *pop                 = nullptr;
    const char *service_key         = nullptr;
    bool        auto_start          = true;
    bool        skip_if_provisioned = true;
};

// ---- capability class ----

class provisioning_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::provisioning;

    explicit provisioning_capability(const provisioning_config &cfg);
    ~provisioning_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::provisioning; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const provisioning_status &status() const { return status_; }

    blusys_err_t request_reset();

private:
    static void prov_event_handler(int event, const void *creds, void *user_ctx);
    void post_event(provisioning_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    provisioning_config cfg_;
    provisioning_status status_{};
    bool first_poll_ = true;
};

}  // namespace blusys
