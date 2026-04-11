#include "core/app_logic.hpp"

#include "blusys/log.h"

#include <cstdio>
#include <ctime>
#include <cstring>

namespace connected_headless {

namespace {

constexpr const char *TAG = "app";

}  // namespace

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    switch (action) {
    case Action::wifi_got_ip:
        state.connected = true;
        if (auto *conn = ctx.connectivity(); conn != nullptr) {
            BLUSYS_LOGI(TAG, "Wi-Fi connected, IP: %s", conn->ip_info.ip);
        }
        break;

    case Action::wifi_disconnected:
        state.connected = false;
        BLUSYS_LOGW(TAG, "Wi-Fi disconnected");
        break;

    case Action::time_synced: {
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

    case Action::capability_ready:
        state.capability_ready = true;
        BLUSYS_LOGI(TAG, "All services ready — product operational");
        break;

    case Action::periodic_tick:
        state.tick_count++;
        if (state.tick_count % 100 == 0) {
            BLUSYS_LOGI(TAG, "heartbeat #%d connected=%s",
                        state.tick_count,
                        state.connected ? "yes" : "no");
        }
        break;
    }
}

bool map_event(std::uint32_t id, std::uint32_t /*code*/,
               const void * /*payload*/, Action *out)
{
    using CE = blusys::app::connectivity_event;
    switch (static_cast<CE>(id)) {
    case CE::wifi_got_ip:
        *out = Action::wifi_got_ip;
        return true;
    case CE::wifi_disconnected:
        *out = Action::wifi_disconnected;
        return true;
    case CE::time_synced:
        *out = Action::time_synced;
        return true;
    case CE::capability_ready:
        *out = Action::capability_ready;
        return true;
    default:
        return false;
    }
}

void on_tick(blusys::app::app_ctx &ctx, State & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(Action::periodic_tick);
}

}  // namespace connected_headless
