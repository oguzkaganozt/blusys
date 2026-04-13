#include "core/app_logic.hpp"

#include "blusys/app/view/bindings.hpp"
#include "blusys/log.h"

namespace handheld_bluetooth {

namespace {

void set_status(State &state, const char *line1, const char *line2)
{
    if (state.status_lbl != nullptr) {
        view_ns::set_text(state.status_lbl, line1);
    }
    if (state.advert_lbl != nullptr) {
        view_ns::set_text(state.advert_lbl, line2);
    }
}

}  // namespace

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    using CET = blusys::app::capability_event_tag;

    switch (action.tag) {
    case Tag::capability_event:
        switch (action.cap_event.tag) {
        case CET::bt_gap_ready:
            state.gap_ready = true;
            set_status(state, "GAP ready", state.advertising ? "Advertising" : "Adv idle");
            BLUSYS_LOGI(kTag, "GAP ready");
            break;
        case CET::bt_advertising_started:
            state.advertising = true;
            set_status(state, state.gap_ready ? "GAP ready" : "GAP…", "Advertising");
            break;
        case CET::bt_advertising_stopped:
            state.advertising = false;
            set_status(state, state.gap_ready ? "GAP ready" : "GAP…", "Adv stopped");
            break;
        case CET::bluetooth_ready:
            state.bt_ready = true;
            if (const auto *st = ctx.bluetooth(); st != nullptr) {
                set_status(state, "Bluetooth ready",
                           st->advertising ? "Advertising" : "Idle");
            }
            BLUSYS_LOGI(kTag, "bluetooth capability ready");
            break;
        default:
            break;
        }
        break;

    case Tag::ping:
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        BLUSYS_LOGI(kTag, "ping");
        break;
    }
}

bool map_intent(blusys::app::app_services &svc, blusys::framework::intent intent, Action *out)
{
    (void)svc;
    switch (intent) {
    case blusys::framework::intent::confirm:
        *out = Action{.tag = Tag::ping};
        return true;
    default:
        return false;
    }
}

}  // namespace handheld_bluetooth
