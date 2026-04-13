#pragma once

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"

#include <cstdint>

namespace lan_control_basic {

enum class action_tag : std::uint8_t {
    capability_event,
};

struct action {
    action_tag tag = action_tag::capability_event;
    blusys::app::capability_event cap_event{};
};

struct app_state {
    bool lan_control_ready = false;
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);

}  // namespace lan_control_basic
