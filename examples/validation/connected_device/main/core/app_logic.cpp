#include "core/app_logic.hpp"

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/hal/log.h"

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

bool map_intent(blusys::app_services &svc, blusys::intent intent, Action *out)
{
    (void)svc;
    switch (intent) {
    case blusys::intent::increment:
        *out = Action{.tag = Tag::brightness_up};
        return true;
    case blusys::intent::decrement:
        *out = Action{.tag = Tag::brightness_down};
        return true;
    case blusys::intent::confirm:
        *out = Action{.tag = Tag::confirm};
        return true;
    default:
        return false;
    }
}

}  // namespace connected_device
