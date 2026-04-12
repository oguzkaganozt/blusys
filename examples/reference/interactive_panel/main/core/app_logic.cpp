#include "core/app_logic.hpp"

#include "blusys_examples/archetype_percent.hpp"
#include "blusys_examples/panel_connectivity_sync.hpp"

#include "blusys/app/view/bindings.hpp"
#include "blusys/app/view/composites.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"

#include <cstdio>

namespace interactive_panel {

namespace {

void sync_shell(app_state &state)
{
    if (state.shell_badge != nullptr) {
        const bool ready = state.diagnostics.capability_ready;
        blusys::app::view::set_badge_text(state.shell_badge, ready ? "Online" : "Warmup");
        blusys::app::view::set_badge_level(
            state.shell_badge,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }

    if (state.shell_detail != nullptr) {
        char detail[48];
        std::snprintf(detail, sizeof(detail), "%s  Q%ld  %ldC",
                      mode_name(state.mode_index),
                      static_cast<long>(state.queue_depth),
                      static_cast<long>(state.temp_c));
        blusys::app::view::set_text(state.shell_detail, detail);
    }
}

void sync_dashboard(app_state &state)
{
    if (state.dash_chart != nullptr && state.dash_series >= 0) {
        blusys::app::view::sync_line_chart_series(
            state.dash_chart,
            state.dash_series,
            state.load_history.data(),
            static_cast<std::int32_t>(state.load_history.size()));
    }

    char v_load[16];
    char v_temp[16];
    std::snprintf(v_load, sizeof(v_load), "%ld%%", static_cast<long>(state.load_percent));
    std::snprintf(v_temp, sizeof(v_temp), "%ld C", static_cast<long>(state.temp_c));

    const blusys::app::view::key_value_trio metrics{
        {state.dash_load, state.dash_temp, state.dash_mode},
    };
    blusys::app::view::sync_key_value_trio(metrics, v_load, v_temp, mode_name(state.mode_index));
    if (state.dash_table != nullptr) {
        char queue[16];
        char heap[16];
        std::snprintf(queue, sizeof(queue), "%ld", static_cast<long>(state.queue_depth));
        std::snprintf(heap, sizeof(heap), "%lu KB",
                      static_cast<unsigned long>(state.diagnostics.last_snapshot.free_heap / 1024));
        blusys::app::view::set_cell_text(state.dash_table, 1, 1, queue);
        blusys::app::view::set_cell_text(state.dash_table, 2, 1, heap);
        blusys::app::view::set_cell_text(state.dash_table, 3, 1,
                                         state.storage_ready ? "Mounted" : "Pending");
    }
}

void sync_status(blusys::app::app_ctx &ctx, app_state &state)
{
    blusys::app::screens::status_screen_update(state.status_handles, ctx);
}

void sync_all(blusys::app::app_ctx &ctx, app_state &state)
{
    sync_shell(state);
    sync_dashboard(state);
    sync_status(ctx, state);
}

}  // namespace

const char *mode_name(std::int32_t index)
{
    switch (index) {
    case 0:
        return "Calm";
    case 1:
        return "Balanced";
    case 2:
        return "Peak";
    default:
        return "Balanced";
    }
}

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    using CET = blusys::app::capability_event_tag;

    switch (event.tag) {
    case action_tag::capability_event: {
        const auto t = event.cap_event.tag;
        if (t == CET::diag_snapshot_ready || t == CET::diagnostics_ready ||
            t == CET::diag_console_ready) {
            if (const auto *diag = ctx.diagnostics(); diag != nullptr) {
                state.diagnostics = *diag;
            }
        } else if (t == CET::storage_ready) {
            if (const auto *storage = ctx.storage(); storage != nullptr) {
                state.storage_ready = storage->capability_ready;
            }
        } else if (blusys_examples::panel_connectivity_event_triggers_sync(t)) {
            // connectivity state read from ctx in status_screen_update
        }
        break;
    }

    case action_tag::sample_tick: {
        ++state.tick_count;
        state.load_percent =
            blusys_examples::clamp_percent(24 + static_cast<std::int32_t>((state.tick_count * 7) % 58));
        state.queue_depth = 2 + static_cast<std::int32_t>(state.tick_count % 7);
        state.temp_c = 25 + static_cast<std::int32_t>((state.tick_count * 3) % 11);
        for (std::size_t i = 1; i < state.load_history.size(); ++i) {
            state.load_history[i - 1] = state.load_history[i];
        }
        state.load_history[state.load_history.size() - 1] = state.load_percent;
        break;
    }

    case action_tag::set_mode:
        state.mode_index = event.value;
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::click);
        break;

    case action_tag::show_dashboard:
        ctx.services().navigate_to(route_dashboard);
        break;

    case action_tag::show_status:
        ctx.services().navigate_to(route_status);
        break;

    case action_tag::show_settings:
        ctx.services().navigate_to(route_settings);
        break;

    case action_tag::open_about:
        ctx.services().navigate_push(route_about);
        break;
    }

    sync_all(ctx, state);
}

bool map_intent(blusys::app::app_services &svc, blusys::framework::intent intent, action *out)
{
    (void)svc;
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = action{.tag = action_tag::set_mode, .value = 2};
        return true;
    case blusys::framework::intent::decrement:
        *out = action{.tag = action_tag::set_mode, .value = 0};
        return true;
    case blusys::framework::intent::confirm:
        *out = action{.tag = action_tag::show_status};
        return true;
    case blusys::framework::intent::cancel:
        *out = action{.tag = action_tag::show_dashboard};
        return true;
    default:
        return false;
    }
}

void on_tick(blusys::app::app_ctx &ctx, blusys::app::app_services &svc, app_state & /*state*/, std::uint32_t /*now_ms*/)
{
    (void)svc;
    ctx.dispatch(action{.tag = action_tag::sample_tick});
}

}  // namespace interactive_panel
