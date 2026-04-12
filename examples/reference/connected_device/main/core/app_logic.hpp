#pragma once

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"

#include <cstdint>

#include "lvgl.h"

namespace connected_device {

constexpr const char *kTag = "conn_dev";

namespace view_ns = blusys::app::view;
using blusys::app::app_ctx;

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
    blusys::app::capability_event   cap_event{};
};

void update(app_ctx &ctx, State &state, const Action &action);

bool map_intent(blusys::framework::intent intent, Action *out);

}  // namespace connected_device
