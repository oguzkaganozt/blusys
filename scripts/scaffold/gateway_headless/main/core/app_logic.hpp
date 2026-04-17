#pragma once

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/event.hpp"

#include <cstdint>
#include <optional>

namespace surface_gateway {

enum class op_state : std::uint8_t {
    idle,
    operational,
};

enum class action_tag : std::uint8_t {
    capability_event,
};

struct action {
    action_tag tag = action_tag::capability_event;
    blusys::capability_event cap_event{};
};

struct app_state {
    op_state      phase = op_state::idle;
    bool          connectivity_ready = false;
    bool          storage_ready = false;
    bool          diagnostics_ready = false;
    bool          telemetry_ready = false;
    bool          ota_ready = false;
    bool          lan_control_ready = false;
    std::uint32_t sample_count = 0;
    std::int32_t  active_devices = 0;
    float         agg_throughput = 0.0f;
    std::uint32_t delivered = 0;
    std::uint32_t delivery_fails = 0;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);
std::optional<action> on_event(blusys::event event, app_state &state);
const char *op_state_name(op_state state);

}  // namespace surface_gateway
