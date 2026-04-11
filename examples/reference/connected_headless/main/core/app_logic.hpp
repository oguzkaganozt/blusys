#pragma once

#include "blusys/app/app.hpp"

#include <cstdint>

namespace connected_headless {

struct State {
    bool connected        = false;
    bool time_synced      = false;
    bool capability_ready = false;
    int  tick_count       = 0;
};

enum class Action {
    wifi_got_ip,
    wifi_disconnected,
    time_synced,
    capability_ready,
    periodic_tick,
};

void update(blusys::app::app_ctx &ctx, State &state, const Action &action);

bool map_event(std::uint32_t id, std::uint32_t code, const void *payload, Action *out);

void on_tick(blusys::app::app_ctx &ctx, State &state, std::uint32_t now_ms);

}  // namespace connected_headless
