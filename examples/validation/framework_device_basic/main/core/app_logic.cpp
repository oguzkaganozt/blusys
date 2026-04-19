#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"


#include <optional>

namespace framework_device_basic {

void update(app_ctx &ctx, AppState &state, const Action &action)
{
    switch (action.tag) {
    case Tag::volume_up:
        if (!state.muted && state.volume < 100) {
            state.volume += 5;
            view_ns::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case Tag::volume_down:
        if (!state.muted && state.volume > 0) {
            state.volume -= 5;
            view_ns::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case Tag::toggle_mute:
        state.muted = !state.muted;
        view_ns::set_enabled(state.slider, !state.muted);
        view_ns::set_text(state.mute_label, state.muted ? "MUTED" : "Volume");
        BLUSYS_LOGI(kTag, "muted=%s", state.muted ? "true" : "false");
        break;

    case Tag::confirm:
        ctx.fx().nav.show_overlay(1);
        ctx.emit_feedback(blusys::feedback_channel::audio,
                          blusys::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed — volume=%ld", static_cast<long>(state.volume));
        break;
    }
}

std::optional<Action> on_event(blusys::event event, AppState &state)
{
    (void)state;
    switch (event.source) {
    case blusys::event_source::intent:
        switch (static_cast<blusys::intent>(event.kind)) {
        case blusys::intent::increment:
            return Action{.tag = Tag::volume_up};
        case blusys::intent::decrement:
            return Action{.tag = Tag::volume_down};
        case blusys::intent::confirm:
            return Action{.tag = Tag::confirm};
        case blusys::intent::cancel:
            return Action{.tag = Tag::toggle_mute};
        default:
            return std::nullopt;
        }
    default:
        return std::nullopt;
    }
}

}  // namespace framework_device_basic
