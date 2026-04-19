#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"


#include <optional>

namespace connected_device {

void update(app_ctx &ctx, State &state, const Action &action)
{
    using CET = blusys::capability_event_tag;

    switch (action.tag) {
    case Tag::capability_event:
        switch (action.cap_event.tag) {
        case CET::wifi_got_ip:
            state.connected = true;
            view_ns::set_text(state.status_lbl, "Connected");
            if (auto *conn = ctx.status_of<blusys::connectivity_capability>(); conn != nullptr) {
                view_ns::set_text(state.ip_lbl, conn->ip_info.ip);
            }
            break;
        case CET::wifi_disconnected:
            state.connected = false;
            view_ns::set_text(state.status_lbl, "Disconnected");
            view_ns::set_text(state.ip_lbl, "---");
            break;
        case CET::connectivity_ready:
            state.ready = true;
            view_ns::set_text(state.status_lbl, "Ready");
            BLUSYS_LOGI(kTag, "device ready");
            break;
        default:
            break;
        }
        break;

    case Tag::brightness_up:
        if (state.brightness < 100) {
            state.brightness += 10;
            view_ns::set_value(state.slider, state.brightness);
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case Tag::brightness_down:
        if (state.brightness > 0) {
            state.brightness -= 10;
            view_ns::set_value(state.slider, state.brightness);
            ctx.emit_feedback(blusys::feedback_channel::haptic,
                              blusys::feedback_pattern::click);
        }
        break;

    case Tag::confirm:
        ctx.emit_feedback(blusys::feedback_channel::audio,
                          blusys::feedback_pattern::confirm);
        BLUSYS_LOGI(kTag, "brightness=%ld", static_cast<long>(state.brightness));
        break;
    }
}

std::optional<Action> on_event(blusys::event event, State &state)
{
    (void)state;
    switch (event.source) {
    case blusys::event_source::intent:
        switch (blusys::event_intent(event)) {
        case blusys::intent::increment:
            return Action{.tag = Tag::brightness_up};
        case blusys::intent::decrement:
            return Action{.tag = Tag::brightness_down};
        case blusys::intent::confirm:
            return Action{.tag = Tag::confirm};
        default:
            return std::nullopt;
        }
    case blusys::event_source::integration: {
        blusys::capability_event ce{};
        if (!blusys::map_integration_event(event.id, event.kind, &ce)) {
            return std::nullopt;
        }
        ce.payload = event.payload;
        return Action{.tag = Tag::capability_event, .cap_event = ce};
    }
    default:
        return std::nullopt;
    }
}

}  // namespace connected_device
