#pragma once

#include <cstdint>
#include <array>

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/screens/status_screen.hpp"
#include "lvgl.h"

namespace interactive_panel {

enum route_id : std::uint32_t {
    route_dashboard = 1,
    route_status,
    route_settings,
    route_about,
};

enum class action_tag : std::uint8_t {
    sample_tick,
    set_mode,
    show_dashboard,
    show_status,
    show_settings,
    open_about,
    sync_diagnostics,
    sync_storage,
    sync_connectivity,
};

struct action {
    action_tag   tag;
    std::int32_t value = 0;
};

struct app_state {
    static constexpr std::size_t history_size = 12;

    std::int32_t mode_index   = 0;
    std::int32_t load_percent = 34;
    std::int32_t queue_depth  = 3;
    std::int32_t temp_c       = 27;
    bool         storage_ready = false;
    std::uint32_t tick_count  = 0;
    std::array<std::int32_t, history_size> load_history{
        28, 30, 32, 35, 34, 37, 40, 42, 39, 36, 35, 34,
    };

    blusys::app::diagnostics_status diagnostics{};

    lv_obj_t *shell_badge    = nullptr;
    lv_obj_t *shell_detail   = nullptr;
    lv_obj_t *dash_chart     = nullptr;
    lv_obj_t *dash_load      = nullptr;
    lv_obj_t *dash_temp      = nullptr;
    lv_obj_t *dash_mode      = nullptr;
    lv_obj_t *dash_table     = nullptr;
    std::int32_t dash_series = -1;

    blusys::app::screens::status_screen_handles status_handles{};
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);
bool map_intent(blusys::framework::intent intent, action *out);
void on_tick(blusys::app::app_ctx &ctx, app_state &state, std::uint32_t now_ms);

const char *mode_name(std::int32_t index);

}  // namespace interactive_panel
