#pragma once

#include <cstdint>

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/flows/provisioning_flow.hpp"
#include "lvgl.h"

namespace interactive_controller {

enum route_id : std::uint32_t {
    route_home = 1,
    route_status,
    route_settings,
    route_setup,
    route_about,
};

enum overlay_id : std::uint32_t {
    overlay_confirm = 1,
};

enum class action_tag : std::uint8_t {
    level_delta,
    toggle_hold,
    set_accent,
    confirm,
    show_home,
    show_status,
    show_settings,
    open_setup,
    open_about,
    sync_storage,
    sync_provisioning,
};

struct action {
    action_tag   tag;
    std::int32_t value = 0;
};

struct app_state {
    std::int32_t level        = 64;
    bool         hold_enabled = false;
    std::int32_t accent_index = 1;
    bool         storage_ready = false;
    blusys::app::provisioning_status provisioning{};

    lv_obj_t *shell_badge      = nullptr;
    lv_obj_t *shell_detail     = nullptr;
    lv_obj_t *home_gauge       = nullptr;
    lv_obj_t *home_level_bar   = nullptr;
    lv_obj_t *home_vu_strip    = nullptr;
    lv_obj_t *home_preset      = nullptr;
    lv_obj_t *home_hold_badge  = nullptr;
    lv_obj_t *status_setup     = nullptr;
    lv_obj_t *status_storage   = nullptr;
    lv_obj_t *status_output    = nullptr;
    lv_obj_t *status_hold      = nullptr;

    blusys::app::flows::provisioning_screen_handles setup_handles{};
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);
bool map_intent(blusys::framework::intent intent, action *out);

const char *accent_name(std::int32_t index);

}  // namespace interactive_controller
