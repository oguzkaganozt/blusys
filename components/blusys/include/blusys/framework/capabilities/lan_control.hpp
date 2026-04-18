#pragma once

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/services/protocol/local_ctrl.h"

#include <cstddef>
#include <cstdint>

// Forward declarations for device-side LAN control and mDNS handles/types.
// Concrete definitions live in the device service headers.
struct blusys_mdns;
typedef struct blusys_mdns blusys_mdns_t;

namespace blusys { class runtime; }

namespace blusys {

// ---- shared types (always available) ----

enum class lan_control_event : std::uint32_t {
    http_ready       = 0x0A00,
    mdns_ready       = 0x0A01,
    capability_ready = 0x0AFF,
};

struct lan_control_status : capability_status_base {
    bool http_listening = false;
    bool mdns_announced = false;
};

// ---- configuration ----

struct lan_control_config {
    const char                       *device_name    = nullptr;
    std::uint16_t                     http_port      = 80;
    const blusys_local_ctrl_action_t *actions        = nullptr;
    std::size_t                       action_count   = 0;
    std::size_t                       max_body_len   = 0;
    std::size_t                       max_response_len = 0;
    blusys_local_ctrl_status_cb_t     status_cb      = nullptr;
    void                             *user_ctx       = nullptr;

    bool        advertise_mdns = true;
    const char *service_name   = "blusys";
    const char *instance_name  = nullptr;
    const char *service_type   = "_http";
    // mDNS protocol: 0 = TCP, 1 = UDP (matches BLUSYS_MDNS_PROTO_TCP/UDP).
    int         service_proto  = 0;
};

// ---- capability class ----

class lan_control_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::lan_control;

    explicit lan_control_capability(const lan_control_config &cfg);
    ~lan_control_capability() override;

    [[nodiscard]] capability_kind kind() const override { return capability_kind::lan_control; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const lan_control_status &status() const { return status_; }

private:
    void check_capability_ready();
    void post_event(lan_control_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    struct impl;
    impl *impl_ = nullptr;

    lan_control_config cfg_;
    lan_control_status status_{};
};

}  // namespace blusys
