#include "core/app_logic.hpp"

#include "blusys/log.h"

namespace headless_lan_control {

namespace {

constexpr const char *kTag = "headless_lan_control";

void sync_lan(blusys::app::app_ctx &ctx, app_state &state)
{
    if (const auto *lc = ctx.lan_control(); lc != nullptr) {
        state.lan_control_ready = lc->capability_ready;
    }
}

}  // namespace

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    if (event.tag != action_tag::capability_event) {
        return;
    }

    using CET = blusys::app::capability_event_tag;
    switch (event.cap_event.tag) {
    case CET::lan_control_ready:
        sync_lan(ctx, state);
        BLUSYS_LOGI(kTag, "lan_control ready (http=%d mdns=%d)",
                    ctx.lan_control() != nullptr && ctx.lan_control()->http_listening ? 1 : 0,
                    ctx.lan_control() != nullptr && ctx.lan_control()->mdns_announced ? 1 : 0);
        break;
    default:
        break;
    }
}

}  // namespace headless_lan_control
