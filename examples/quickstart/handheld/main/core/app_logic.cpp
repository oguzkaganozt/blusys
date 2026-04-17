#include "core/app_logic.hpp"

#include "blusys_examples/clamp_percent.hpp"
#include "blusys/blusys.hpp"


#include <cstdint>
#include <optional>

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
            if (const auto *storage = ctx.status_of<blusys::storage_capability>(); storage != nullptr) {
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
            if (const auto *conn = ctx.status_of<blusys::connectivity_capability>(); conn != nullptr) {
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
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case action_tag::toggle_hold:
        state.hold_enabled = !state.hold_enabled;
        ctx.emit_feedback(blusys::feedback_channel::haptic,
                          state.hold_enabled
                              ? blusys::feedback_pattern::warning
                              : blusys::feedback_pattern::click);
        break;

    case action_tag::set_accent:
        state.accent_index = event.value;
        ctx.emit_feedback(blusys::feedback_channel::audio,
                          blusys::feedback_pattern::click);
        break;

    case action_tag::confirm:
        ctx.fx().nav.show_overlay(overlay::confirm);
        ctx.emit_feedback(blusys::feedback_channel::audio,
                          blusys::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed level=%ld accent=%s hold=%s",
                    static_cast<long>(state.level),
                    accent_name(state.accent_index),
                    state.hold_enabled ? "yes" : "no");
        break;

    case action_tag::show_home:
        ctx.fx().nav.to(route::home);
        break;

    case action_tag::show_status:
        ctx.fx().nav.to(route::status);
        break;

    case action_tag::show_settings:
        ctx.fx().nav.to(route::settings);
        break;

    case action_tag::open_setup:
        ctx.fx().nav.push(route::setup);
        break;

    case action_tag::open_about:
        ctx.fx().nav.push(route::about);
        break;
    }

    ui::sync_all_panels(state);
}

std::optional<action> on_event(blusys::event event, app_state &state)
{
    (void)state;
    switch (event.kind) {
    case blusys::app_event_kind::intent:
        switch (blusys::event_intent(event)) {
        case blusys::intent::increment:
            return action{.tag = action_tag::level_delta, .value = 4};
        case blusys::intent::decrement:
            return action{.tag = action_tag::level_delta, .value = -4};
        case blusys::intent::confirm:
            return action{.tag = action_tag::confirm};
        case blusys::intent::cancel:
            return action{.tag = action_tag::toggle_hold};
        default:
            return std::nullopt;
        }
    case blusys::app_event_kind::integration: {
        blusys::capability_event ce{};
        if (!blusys::map_integration_event(event.id, event.code, &ce)) {
            return std::nullopt;
        }
        ce.payload = event.payload;
        return action{.tag = action_tag::capability_event, .cap_event = ce};
    }
    default:
        return std::nullopt;
    }
}

}  // namespace handheld_starter
