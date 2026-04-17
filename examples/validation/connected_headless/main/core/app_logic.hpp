#pragma once
#include "blusys/blusys.hpp"


#include <cstdint>
#include <optional>

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
    blusys::capability_event   cap_event{};
};

void update(blusys::app_ctx &ctx, State &state, const Action &action);
std::optional<Action> on_event(blusys::event event, State &state);

void on_tick(blusys::app_ctx &ctx,  blusys::app_fx &fx, State &state, std::uint32_t now_ms);

}  // namespace connected_headless
