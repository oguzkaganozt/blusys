#pragma once

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/event.hpp"

#include <cstdint>

#include "lvgl.h"

namespace handheld_bluetooth {

constexpr const char *kTag = "handheld_bt";

namespace view_ns = blusys::app::view;

struct State {
    bool     gap_ready        = false;
    bool     advertising    = false;
    bool     bt_ready         = false;
    lv_obj_t *status_lbl      = nullptr;
    lv_obj_t *advert_lbl      = nullptr;
};

enum class Tag : std::uint8_t {
    capability_event,
    ping,
};

struct Action {
    Tag                             tag{};
    blusys::app::capability_event   cap_event{};
};

void update(blusys::app::app_ctx &ctx, State &state, const Action &action);

bool map_intent(blusys::app::app_services &svc, blusys::framework::intent intent, Action *out);

}  // namespace handheld_bluetooth
