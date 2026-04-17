#pragma once

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/event.hpp"

#include <cstdint>
#include <optional>

namespace telemetry_lp {

enum class action_tag : std::uint8_t {
    capability_event,
};

struct action {
    action_tag tag = action_tag::capability_event;
    blusys::capability_event cap_event{};
};

struct app_state {
    bool connectivity_ready = false;
    bool telemetry_ready    = false;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);
std::optional<action> on_event(blusys::event event, app_state &state);

}  // namespace telemetry_lp
