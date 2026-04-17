#pragma once

#include "blusys/framework/capabilities/capability.hpp"

#include "blusys/framework/services/net.h"

#include <cstddef>
#include <cstdint>

namespace blusys { class runtime; }

namespace blusys {

enum class lan_control_event : std::uint32_t {
    http_ready      = 0x0A00,
    mdns_ready      = 0x0A01,
    capability_ready = 0x0AFF,
};

struct lan_control_status : capability_status_base {
    bool http_listening  = false;
    bool mdns_announced  = false;
};

struct lan_control_config {
    const char *device_name = nullptr;
    std::uint16_t http_port = 80;
    const blusys_local_ctrl_action_t *actions = nullptr;
    std::size_t action_count = 0;
    std::size_t max_body_len = 0;
    std::size_t max_response_len = 0;
    blusys_local_ctrl_status_cb_t status_cb = nullptr;
    void *user_ctx = nullptr;

    bool advertise_mdns = true;
    const char *service_name = "blusys";
    const char *instance_name = nullptr;
    const char *service_type = "_http";
    blusys_mdns_proto_t service_proto = BLUSYS_MDNS_PROTO_TCP;
};

#ifdef ESP_PLATFORM

class lan_control_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::lan_control;

    explicit lan_control_capability(const lan_control_config &cfg);

    [[nodiscard]] capability_kind kind() const override { return capability_kind::lan_control; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const lan_control_status &status() const { return status_; }

private:
    void post_event(lan_control_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }
    void check_capability_ready();

    lan_control_config cfg_;
    lan_control_status status_{};
    blusys_local_ctrl_t *ctrl_ = nullptr;
    blusys_mdns_t *mdns_ = nullptr;
};

#else

class lan_control_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::lan_control;

    explicit lan_control_capability(const lan_control_config &cfg)
        : cfg_(cfg) {}

    [[nodiscard]] capability_kind kind() const override { return capability_kind::lan_control; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

    [[nodiscard]] const lan_control_status &status() const { return status_; }

private:
    void post_event(lan_control_event ev)
    {
        post_integration_event(static_cast<std::uint32_t>(ev));
    }

    lan_control_config cfg_;
    lan_control_status status_{};
    bool first_poll_ = true;
};

#endif

}  // namespace blusys
