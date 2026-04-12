#include "core/app_logic.hpp"

#include "blusys_examples/archetype_percent.hpp"

#include "blusys/app/view/bindings.hpp"
#include "blusys/app/view/composites.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/widgets/chart/chart.hpp"

#include <cstdio>

namespace interactive_dashboard {

namespace {

void sync_shell(app_state &state)
{
    if (state.shell_badge != nullptr) {
        const bool ready = state.diagnostics.capability_ready;
        blusys::app::view::set_badge_text(state.shell_badge, ready ? "Live" : "Init");
        blusys::app::view::set_badge_level(
            state.shell_badge,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }

    if (state.shell_detail != nullptr) {
        char detail[48];
        std::snprintf(detail, sizeof(detail), "%s  sp%ld  Q%ld",
                      mode_name(state.mode_index),
                      static_cast<long>(state.target_setpoint),
                      static_cast<long>(state.queue_depth));
        blusys::app::view::set_text(state.shell_detail, detail);
    }
}

void sync_overview(app_state &state)
{
    char v_ops[24];
    char v_temp[16];
    char v_q[20];
    const std::int32_t ops = 1200 + (state.load_percent * 13) + static_cast<std::int32_t>(state.tick_count % 40);
    if (ops >= 1000) {
        std::snprintf(v_ops, sizeof(v_ops), "%.1fk ops/m", static_cast<double>(ops) / 1000.0);
    } else {
        std::snprintf(v_ops, sizeof(v_ops), "%ld ops/m", static_cast<long>(ops));
    }
    std::snprintf(v_temp, sizeof(v_temp), "%ld C", static_cast<long>(state.temp_c));
    std::snprintf(v_q, sizeof(v_q), "%ld jobs", static_cast<long>(state.queue_depth));

    const blusys::app::view::key_value_trio kpis{
        {state.kpi_ops, state.kpi_temp, state.kpi_queue},
    };
    blusys::app::view::sync_key_value_trio(kpis, v_ops, v_temp, v_q);

    if (state.dash_gauge != nullptr) {
        blusys::app::view::set_gauge_value(state.dash_gauge, state.load_percent);
    }

    if (state.dash_line_chart != nullptr && state.line_series >= 0) {
        blusys::app::view::sync_line_chart_series(
            state.dash_line_chart,
            state.line_series,
            state.load_history.data(),
            static_cast<std::int32_t>(state.load_history.size()));
    }

    if (state.dash_bar_chart != nullptr && state.bar_series >= 0) {
        blusys::ui::chart_set_all(state.dash_bar_chart,
                                  state.bar_series,
                                  state.zone_values.data(),
                                  static_cast<std::int32_t>(state.zone_values.size()));
        blusys::ui::chart_refresh(state.dash_bar_chart);
    }
}

void sync_all(app_state &state)
{
    sync_shell(state);
    sync_overview(state);
}

}  // namespace

const char *mode_name(std::int32_t index)
{
    switch (index) {
    case 0:
        return "Eco";
    case 1:
        return "Balanced";
    case 2:
        return "Boost";
    default:
        return "Balanced";
    }
}

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    using CET = blusys::app::capability_event_tag;

    switch (event.tag) {
    case action_tag::capability_event:
        switch (event.cap_event.tag) {
        case CET::diag_snapshot_ready:
        case CET::diagnostics_ready:
        case CET::diag_console_ready:
            if (const auto *diag = ctx.diagnostics(); diag != nullptr) {
                state.diagnostics = *diag;
            }
            break;
        case CET::storage_ready:
            if (const auto *storage = ctx.storage(); storage != nullptr) {
                state.storage_ready = storage->capability_ready;
            }
            break;
        default:
            break;
        }
        break;

    case action_tag::sample_tick: {
        ++state.tick_count;
        const std::int32_t wobble =
            static_cast<std::int32_t>((state.tick_count * 7) % 23) - 11;
        const std::int32_t target_pull =
            (state.target_setpoint - state.load_percent) / 8;
        state.load_percent = blusys_examples::clamp_percent(state.load_percent + wobble / 3 +
                                                            target_pull);
        state.queue_depth  = 2 + static_cast<std::int32_t>(state.tick_count % 9);
        state.temp_c       = 24 + static_cast<std::int32_t>((state.tick_count * 2) % 14);

        for (std::size_t i = 1; i < state.load_history.size(); ++i) {
            state.load_history[i - 1] = state.load_history[i];
        }
        state.load_history[state.load_history.size() - 1] = state.load_percent;

        for (std::size_t z = 0; z < state.zone_values.size(); ++z) {
            const std::int32_t phase =
                static_cast<std::int32_t>((state.tick_count + z * 5) % 37);
            state.zone_values[z] =
                blusys_examples::clamp_percent(20 + phase + static_cast<std::int32_t>(z * 6));
        }
        break;
    }

    case action_tag::set_mode:
        state.mode_index = event.value;
        if (state.mode_index < 0) {
            state.mode_index = 0;
        }
        if (state.mode_index > 2) {
            state.mode_index = 2;
        }
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::click);
        break;

    case action_tag::set_target:
        state.target_setpoint = blusys_examples::clamp_percent(event.value);
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        break;

    case action_tag::nudge_load:
        state.load_percent =
            blusys_examples::clamp_percent(state.load_percent + event.value);
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        break;

    case action_tag::show_overview:
        ctx.navigate_to(route_overview);
        break;

    case action_tag::show_about:
        ctx.navigate_to(route_about);
        break;
    }

    sync_all(state);
}

bool map_intent(blusys::framework::intent intent, action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = action{.tag = action_tag::nudge_load, .value = 8};
        return true;
    case blusys::framework::intent::decrement:
        *out = action{.tag = action_tag::nudge_load, .value = -8};
        return true;
    case blusys::framework::intent::confirm:
        *out = action{.tag = action_tag::show_about};
        return true;
    case blusys::framework::intent::cancel:
        *out = action{.tag = action_tag::show_overview};
        return true;
    default:
        return false;
    }
}

void on_tick(blusys::app::app_ctx &ctx, app_state & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(action{.tag = action_tag::sample_tick});
}

}  // namespace interactive_dashboard
