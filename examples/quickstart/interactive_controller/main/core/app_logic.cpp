#include "core/app_logic.hpp"

#include "blusys_examples/archetype_percent.hpp"

#include "blusys/log.h"

#include <cstdint>

namespace interactive_controller {

namespace {

constexpr const char *kTag = "ctrl_archetype";

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
        ctx.show_overlay(overlay::confirm);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed level=%ld accent=%s hold=%s",
                    static_cast<long>(state.level),
                    accent_name(state.accent_index),
                    state.hold_enabled ? "yes" : "no");
        break;

    case action_tag::show_home:
        ctx.navigate_to(route::home);
        break;

    case action_tag::show_status:
        ctx.navigate_to(route::status);
        break;

    case action_tag::show_settings:
        ctx.navigate_to(route::settings);
        break;

    case action_tag::open_setup:
        ctx.navigate_push(route::setup);
        break;

    case action_tag::open_about:
        ctx.navigate_push(route::about);
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

    ui::sync_all_panels(state);
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
