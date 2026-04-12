#pragma once

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"

#include <cstdint>

namespace connected_headless {

struct State {
    bool connected        = false;
    bool time_synced      = false;
    bool capability_ready = false;
    int  tick_count       = 0;
};

enum class action_tag : std::uint8_t {
    capability_event,
    periodic_tick,
};

struct Action {
    action_tag                      tag = action_tag::periodic_tick;
    blusys::app::capability_event   cap_event{};
};

void update(blusys::app::app_ctx &ctx, State &state, const Action &action);

void on_tick(blusys::app::app_ctx &ctx,  blusys::app::app_services &svc, State &state, std::uint32_t now_ms);

}  // namespace connected_headless
