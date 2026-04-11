#include "core/app_logic.hpp"

#include "blusys_examples/archetype_percent.hpp"

#include "blusys/app/flows/provisioning_flow.hpp"
#include "blusys/app/view/bindings.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "blusys/framework/ui/widgets/gauge/gauge.hpp"
#include "blusys/framework/ui/widgets/level_bar/level_bar.hpp"
#include "blusys/framework/ui/widgets/vu_strip/vu_strip.hpp"
#include "blusys/log.h"

#include <cstdint>
#include <cstdio>

namespace interactive_controller {

namespace {

constexpr const char *kTag = "ctrl_archetype";

void sync_shell(app_state &state)
{
    if (state.shell_badge != nullptr) {
        const bool setup_ready = state.provisioning.capability_ready || state.provisioning.is_provisioned;
        blusys::app::view::set_badge_text(state.shell_badge, setup_ready ? "Ready" : "Setup");
        blusys::app::view::set_badge_level(
            state.shell_badge,
            setup_ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }

    if (state.shell_detail != nullptr) {
        char detail[48];
        std::snprintf(detail, sizeof(detail), "%s  %ld%%",
                      accent_name(state.accent_index),
                      static_cast<long>(state.level));
        blusys::app::view::set_text(state.shell_detail, detail);
    }
}

void sync_home(app_state &state)
{
    if (state.home_gauge != nullptr) {
        blusys::ui::gauge_set_value(state.home_gauge, state.level);
    }
    if (state.home_level_bar != nullptr) {
        blusys::ui::level_bar_set_value(state.home_level_bar, state.level);
    }
    if (state.home_vu_strip != nullptr) {
        constexpr std::int32_t kSeg = 12;
        const auto lit = static_cast<std::uint8_t>((state.level * kSeg) / 100);
        blusys::ui::vu_strip_set_value(state.home_vu_strip, lit);
    }
    if (state.home_preset != nullptr) {
        blusys::app::view::set_kv_value(state.home_preset, accent_name(state.accent_index));
    }
    if (state.home_hold_badge != nullptr) {
        blusys::app::view::set_badge_text(state.home_hold_badge,
                                          state.hold_enabled ? "Hold" : "Live");
        blusys::app::view::set_badge_level(
            state.home_hold_badge,
            state.hold_enabled ? blusys::ui::badge_level::warning
                               : blusys::ui::badge_level::success);
    }
}

void sync_status(app_state &state)
{
    if (state.status_setup != nullptr) {
        const bool ready = state.provisioning.capability_ready || state.provisioning.is_provisioned;
        blusys::app::view::set_badge_text(state.status_setup, ready ? "Provisioned" : "Waiting");
        blusys::app::view::set_badge_level(
            state.status_setup,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }
    if (state.status_storage != nullptr) {
        blusys::app::view::set_kv_value(state.status_storage,
                                        state.storage_ready ? "Mounted" : "Pending");
    }
    if (state.status_output != nullptr) {
        char value[16];
        std::snprintf(value, sizeof(value), "%ld%%", static_cast<long>(state.level));
        blusys::app::view::set_kv_value(state.status_output, value);
    }
    if (state.status_hold != nullptr) {
        blusys::app::view::set_kv_value(state.status_hold,
                                        state.hold_enabled ? "Enabled" : "Off");
    }
}

void sync_setup(app_state &state)
{
    blusys::app::flows::provisioning_screen_update(state.setup_handles, state.provisioning);
}

void sync_all(app_state &state)
{
    sync_shell(state);
    sync_home(state);
    sync_status(state);
    sync_setup(state);
}

}  // namespace

const char *accent_name(std::int32_t index)
{
    switch (index) {
    case 0:
        return "Warm";
    case 1:
        return "Punch";
    case 2:
        return "Night";
    default:
        return "Punch";
    }
}

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    switch (event.tag) {
    case action_tag::level_delta:
        if (!state.hold_enabled) {
            state.level = blusys_examples::clamp_percent(state.level + event.value);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case action_tag::toggle_hold:
        state.hold_enabled = !state.hold_enabled;
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          state.hold_enabled
                              ? blusys::framework::feedback_pattern::warning
                              : blusys::framework::feedback_pattern::click);
        break;

    case action_tag::set_accent:
        state.accent_index = event.value;
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::click);
        break;

    case action_tag::confirm:
        ctx.show_overlay(overlay_confirm);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed level=%ld accent=%s hold=%s",
                    static_cast<long>(state.level),
                    accent_name(state.accent_index),
                    state.hold_enabled ? "yes" : "no");
        break;

    case action_tag::show_home:
        ctx.navigate_to(route_home);
        break;

    case action_tag::show_status:
        ctx.navigate_to(route_status);
        break;

    case action_tag::show_settings:
        ctx.navigate_to(route_settings);
        break;

    case action_tag::open_setup:
        ctx.navigate_push(route_setup);
        break;

    case action_tag::open_about:
        ctx.navigate_push(route_about);
        break;

    case action_tag::sync_storage:
        if (const auto *storage = ctx.storage(); storage != nullptr) {
            state.storage_ready = storage->capability_ready;
        }
        break;

    case action_tag::sync_provisioning:
        if (const auto *prov = ctx.provisioning(); prov != nullptr) {
            state.provisioning = *prov;
        }
        break;
    }

    sync_all(state);
}

bool map_intent(blusys::framework::intent intent, action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = action{.tag = action_tag::level_delta, .value = 4};
        return true;
    case blusys::framework::intent::decrement:
        *out = action{.tag = action_tag::level_delta, .value = -4};
        return true;
    case blusys::framework::intent::confirm:
        *out = action{.tag = action_tag::confirm};
        return true;
    case blusys::framework::intent::cancel:
        *out = action{.tag = action_tag::toggle_hold};
        return true;
    default:
        return false;
    }
}

}  // namespace interactive_controller
