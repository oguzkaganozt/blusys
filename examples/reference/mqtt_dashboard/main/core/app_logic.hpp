#pragma once

#include "blusys/app/capabilities/diagnostics.hpp"

#include "blusys/app/app.hpp"
#include "lvgl.h"

#include <cstdint>

namespace mqtt_dashboard {

enum route_id : std::uint32_t {
    route_live = 1,
    route_about,
};

enum class action_tag : std::uint8_t {
    mqtt_refresh,
    publish_ping,
    publish_toggle,
    publish_scene,
    sync_diagnostics,
    show_live,
    show_about,
};

struct action {
    action_tag   tag;
    std::int32_t value = 0;
};

struct app_state {
    bool         mqtt_ready   = false;
    bool         toggle_state = false;
    std::int32_t scene_id     = 1;

    blusys::app::diagnostics_status diagnostics{};

    lv_obj_t *shell_badge  = nullptr;
    lv_obj_t *shell_detail = nullptr;

    lv_obj_t *status_line  = nullptr;
    lv_obj_t *last_rx_line = nullptr;
    lv_obj_t *metrics_line = nullptr;

    // Root returned from create_live (page.content in shell). Used in on_live_hide so we only
    // clear label ptrs when hiding that instance — reload Live→Live creates a new page before hide,
    // so the outgoing `screen` no longer matches and we must not wipe the new label pointers.
    lv_obj_t *live_screen_root = nullptr;
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);
bool map_intent(blusys::framework::intent intent, action *out);
bool map_event(std::uint32_t id, std::uint32_t /*code*/, const void * /*payload*/, action *out);

}  // namespace mqtt_dashboard
