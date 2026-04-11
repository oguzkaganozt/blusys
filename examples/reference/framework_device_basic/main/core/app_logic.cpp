#include "core/app_logic.hpp"

#include "blusys/log.h"

namespace framework_device_basic {

void update(app_ctx &ctx, AppState &state, const Action &action)
{
    switch (action.tag) {
    case Tag::volume_up:
        if (!state.muted && state.volume < 100) {
            state.volume += 5;
            view_ns::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case Tag::volume_down:
        if (!state.muted && state.volume > 0) {
            state.volume -= 5;
            view_ns::set_value(state.slider, state.volume);
            ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                              blusys::framework::feedback_pattern::click);
        }
        break;

    case Tag::toggle_mute:
        state.muted = !state.muted;
        view_ns::set_enabled(state.slider, !state.muted);
        view_ns::set_text(state.mute_label, state.muted ? "MUTED" : "Volume");
        BLUSYS_LOGI(kTag, "muted=%s", state.muted ? "true" : "false");
        break;

    case Tag::confirm:
        ctx.show_overlay(1);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "confirmed — volume=%ld", static_cast<long>(state.volume));
        break;
    }
}

bool map_intent(blusys::framework::intent intent, Action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = Action{.tag = Tag::volume_up};
        return true;
    case blusys::framework::intent::decrement:
        *out = Action{.tag = Tag::volume_down};
        return true;
    case blusys::framework::intent::confirm:
        *out = Action{.tag = Tag::confirm};
        return true;
    case blusys::framework::intent::cancel:
        *out = Action{.tag = Tag::toggle_mute};
        return true;
    default:
        return false;
    }
}

}  // namespace framework_device_basic
