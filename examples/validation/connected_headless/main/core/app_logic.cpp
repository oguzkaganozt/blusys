#include "core/app_logic.hpp"

#include "blusys/hal/log.h"

#include <cstdio>
#include <ctime>
#include <cstring>

namespace connected_headless {

namespace {

constexpr const char *TAG = "app";

}  // namespace

void update(blusys::app_ctx &ctx, State &state, const Action &action)
{
    using CET = blusys::capability_event_tag;

    switch (action.tag) {
    case action_tag::capability_event:
        switch (action.cap_event.tag) {
        case CET::wifi_got_ip:
            state.connected = true;
            if (auto *conn = ctx.connectivity(); conn != nullptr) {
                BLUSYS_LOGI(TAG, "Wi-Fi connected, IP: %s", conn->ip_info.ip);
            }
            break;
        case CET::wifi_disconnected:
            state.connected = false;
            BLUSYS_LOGW(TAG, "Wi-Fi disconnected");
            break;
        case CET::time_synced: {
            state.time_synced = true;
            std::time_t now;
            std::time(&now);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
            BLUSYS_LOGI(TAG, "Time synced: %s UTC", buf);
            break;
        }
        case CET::connectivity_ready:
            state.capability_ready = true;
            BLUSYS_LOGI(TAG, "All services ready — product operational");
            break;
        default:
            break;
        }
        break;

    case action_tag::periodic_tick:
        state.tick_count++;
        if (state.tick_count % 100 == 0) {
            BLUSYS_LOGI(TAG, "heartbeat #%d connected=%s",
                        state.tick_count,
                        state.connected ? "yes" : "no");
        }
        break;
    }
}

void on_tick(blusys::app_ctx &ctx, blusys::app_services &svc, State & /*state*/, std::uint32_t /*now_ms*/)
{
    (void)svc;
    ctx.dispatch(Action{.tag = action_tag::periodic_tick});
}

}  // namespace connected_headless
