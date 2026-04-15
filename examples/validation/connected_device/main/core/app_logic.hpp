#pragma once

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/event.hpp"

#include <cstdint>

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

bool map_intent(blusys::app_services &svc, blusys::framework::intent intent, Action *out);

}  // namespace connected_device
