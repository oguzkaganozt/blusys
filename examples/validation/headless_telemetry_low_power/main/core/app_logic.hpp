#pragma once

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/event.hpp"

#include <cstdint>

namespace telemetry_lp {

enum class action_tag : std::uint8_t {
    capability_event,
};

struct action {
    action_tag tag = action_tag::capability_event;
    blusys::app::capability_event cap_event{};
};

struct app_state {
    bool connectivity_ready = false;
    bool telemetry_ready    = false;
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);

}  // namespace telemetry_lp
