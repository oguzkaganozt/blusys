#include "core/app_logic.hpp"

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/hal/log.h"

#include <optional>

namespace telemetry_lp {

namespace {

constexpr const char *kTag = "htlm_lp";

void sync_status(blusys::app_ctx &ctx, app_state &state)
{
    if (const auto *c = ctx.status_of<blusys::connectivity_capability>(); c != nullptr) {
        state.connectivity_ready = c->capability_ready;
    }
    if (const auto *t = ctx.status_of<blusys::telemetry_capability>(); t != nullptr) {
        state.telemetry_ready = t->capability_ready;
    }
}

}  // namespace

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{
    if (event.tag != action_tag::capability_event) {
        return;
    }

    using CET = blusys::capability_event_tag;
    switch (event.cap_event.tag) {
    case CET::wifi_got_ip:
    case CET::connectivity_ready:
    case CET::telemetry_ready:
        sync_status(ctx, state);
        BLUSYS_LOGI(kTag, "cap event tag=%u  conn_ready=%d  tel_ready=%d",
                    static_cast<unsigned>(event.cap_event.tag),
                    state.connectivity_ready ? 1 : 0,
                    state.telemetry_ready ? 1 : 0);
        break;
    default:
        break;
    }
}

std::optional<action> on_event(blusys::event event, app_state &state)
{
    (void)state;
    if (event.kind != blusys::app_event_kind::integration) {
        return std::nullopt;
    }

    blusys::capability_event ce{};
    if (!blusys::map_integration_event(event.id, event.code, &ce)) {
        return std::nullopt;
    }
    ce.payload = event.payload;
    return action{.tag = action_tag::capability_event, .cap_event = ce};
}

}  // namespace telemetry_lp
