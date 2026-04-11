#pragma once

#include "blusys/app/app.hpp"

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
    wifi_got_ip,
    wifi_disconnected,
    capability_ready,
    brightness_up,
    brightness_down,
    confirm,
};

struct Action {
    Tag tag;
};

void update(app_ctx &ctx, State &state, const Action &action);

bool map_intent(blusys::framework::intent intent, Action *out);

bool map_event(std::uint32_t id, std::uint32_t code, const void *payload, Action *out);

}  // namespace connected_device
