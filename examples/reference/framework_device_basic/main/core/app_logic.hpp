#pragma once

#include "blusys/app/app.hpp"

#include <cstdint>

#include "lvgl.h"

namespace framework_device_basic {

constexpr const char *kTag = "volume_app";

namespace view_ns = blusys::app::view;
using blusys::app::app_ctx;

struct AppState {
    std::int32_t volume = 50;
    bool         muted  = false;
    lv_obj_t    *slider     = nullptr;
    lv_obj_t    *mute_label = nullptr;
};

enum class Tag : std::uint8_t {
    volume_up,
    volume_down,
    toggle_mute,
    confirm,
};

struct Action {
    Tag tag;
};

void update(app_ctx &ctx, AppState &state, const Action &action);

bool map_intent(blusys::framework::intent intent, Action *out);

}  // namespace framework_device_basic
