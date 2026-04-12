#pragma once

#include <array>
#include <cstdint>

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "lvgl.h"

namespace interactive_dashboard {

enum route_id : std::uint32_t {
    route_overview = 1,
    route_about,
};

enum class action_tag : std::uint8_t {
    capability_event,
    sample_tick,
    set_mode,
    set_target,
    nudge_load,
    show_overview,
    show_about,
};

struct action {
    action_tag                      tag = action_tag::sample_tick;
    std::int32_t                    value = 0;
    blusys::app::capability_event   cap_event{};
};

struct app_state {
    static constexpr std::size_t history_size = 16;
    static constexpr std::size_t zone_count   = 6;

    std::int32_t mode_index      = 1;
    std::int32_t load_percent    = 42;
    std::int32_t target_setpoint = 55;
    std::int32_t queue_depth     = 3;
    std::int32_t temp_c          = 28;
    std::uint32_t tick_count     = 0;
    bool         storage_ready   = false;

    std::array<std::int32_t, history_size> load_history{
        36, 38, 40, 41, 42, 43, 44, 43, 42, 41, 42, 43, 42, 41, 42, 42,
    };
    std::array<std::int32_t, zone_count> zone_values{18, 42, 65, 50, 33, 72};

    blusys::app::diagnostics_status diagnostics{};

    lv_obj_t *shell_badge  = nullptr;
    lv_obj_t *shell_detail = nullptr;

    lv_obj_t *kpi_ops   = nullptr;
    lv_obj_t *kpi_temp  = nullptr;
    lv_obj_t *kpi_queue = nullptr;

    lv_obj_t *dash_gauge      = nullptr;
    lv_obj_t *dash_line_chart = nullptr;
    lv_obj_t *dash_bar_chart  = nullptr;
    std::int32_t line_series  = -1;
    std::int32_t bar_series   = -1;
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);
bool map_intent(blusys::framework::intent intent, action *out);
void on_tick(blusys::app::app_ctx &ctx, app_state &state, std::uint32_t now_ms);

const char *mode_name(std::int32_t index);

}  // namespace interactive_dashboard
