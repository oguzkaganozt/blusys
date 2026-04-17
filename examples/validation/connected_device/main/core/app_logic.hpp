#pragma once
#include "blusys/blusys.hpp"


#include <cstdint>
#include <optional>

#include "lvgl.h"

namespace connected_device {

constexpr const char *kTag = "conn_dev";

namespace view_ns = blusys;
using blusys::app_ctx;

struct State {
    bool         connected  = false;
    bool         ready      = false;
    std::int32_t brightness = 50;
    lv_obj_t    *status_lbl = nullptr;
    lv_obj_t    *ip_lbl     = nullptr;
    lv_obj_t    *slider     = nullptr;
};

enum class Tag : std::uint8_t {
    capability_event,
    brightness_up,
    brightness_down,
    confirm,
};

struct Action {
    Tag                             tag{};
    blusys::capability_event   cap_event{};
};

void update(app_ctx &ctx, State &state, const Action &action);
std::optional<Action> on_event(blusys::event event, State &state);

}  // namespace connected_device
