#pragma once
#include "blusys/blusys.hpp"


#include <cstdint>
#include <optional>

#include "lvgl.h"

namespace framework_device_basic {

constexpr const char *kTag = "volume_app";

namespace view_ns = blusys;
using blusys::app_ctx;

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
std::optional<Action> on_event(blusys::event event, AppState &state);

}  // namespace framework_device_basic
