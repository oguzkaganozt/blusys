#include "core/app_logic.hpp"

#include "blusys_examples/clamp_percent.hpp"

#include "blusys/hal/log.h"

#include <cstdint>

namespace handheld_starter {

namespace {

constexpr const char *kTag = "handheld_starter";

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

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{
    using CET = blusys::capability_event_tag;

    switch (event.tag) {
    case action_tag::capability_event:
        switch (event.cap_event.tag) {
        case CET::storage_ready:
            if (const auto *storage = ctx.storage(); storage != nullptr) {
                state.storage_ready = storage->capability_ready;
            }
            break;
        case CET::prov_started:
        case CET::prov_credentials_received:
        case CET::prov_success:
        case CET::prov_failed:
        case CET::prov_already_done:
        case CET::prov_reset_complete:
        case CET::provisioning_ready:
            if (const auto *conn = ctx.connectivity(); conn != nullptr) {
                state.provisioning = conn->provisioning;
            }
            break;
        default:
            break;
        }
        break;

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
        ctx.services().show_overlay(overlay::confirm);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed level=%ld accent=%s hold=%s",
                    static_cast<long>(state.level),
                    accent_name(state.accent_index),
                    state.hold_enabled ? "yes" : "no");
        break;

    case action_tag::show_home:
        ctx.services().navigate_to(route::home);
        break;

    case action_tag::show_status:
        ctx.services().navigate_to(route::status);
        break;

    case action_tag::show_settings:
        ctx.services().navigate_to(route::settings);
        break;

    case action_tag::open_setup:
        ctx.services().navigate_push(route::setup);
        break;

    case action_tag::open_about:
        ctx.services().navigate_push(route::about);
        break;
    }

    ui::sync_all_panels(state);
}

bool map_intent(blusys::app_services &svc, blusys::framework::intent intent, action *out)
{
    (void)svc;
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

}  // namespace handheld_starter
