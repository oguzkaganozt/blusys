// connected_headless_host — connected headless product running on host.
//
// Same reducer logic as the device connected_headless example, with
// simulated connectivity and storage capabilities. The host stubs post
// integration events on the first poll so the reducer exercises the same
// bridge as on-device.

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/log.h"

#include <cstdint>

namespace {

constexpr const char *kTag = "app";

// ---- state ----

struct State {
    bool saw_wifi_started    = false;
    bool saw_wifi_connecting = false;
    bool connected           = false;
    bool time_synced         = false;
    bool mdns_ready          = false;
    bool conn_capability_ready = false;
    bool storage_ready       = false;
    bool saw_spiffs          = false;
    int  tick_count          = 0;
};

// ---- actions ----

enum class action_tag : std::uint8_t {
    capability_event,
    periodic_tick,
};

struct Action {
    action_tag                      tag = action_tag::periodic_tick;
    blusys::app::capability_event   cap_event{};
};

// ---- reducer ----

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    using CET = blusys::app::capability_event_tag;

    switch (action.tag) {
    case action_tag::capability_event:
        switch (action.cap_event.tag) {
        case CET::wifi_started:
            state.saw_wifi_started = true;
            BLUSYS_LOGI(kTag, "wifi started");
            break;
        case CET::wifi_connecting:
            state.saw_wifi_connecting = true;
            BLUSYS_LOGI(kTag, "wifi connecting");
            break;
        case CET::wifi_got_ip:
            state.connected = true;
            if (auto *conn = ctx.connectivity(); conn != nullptr) {
                BLUSYS_LOGI(kTag, "Wi-Fi connected, IP: %s", conn->ip_info.ip);
            }
            break;
        case CET::wifi_disconnected:
            state.connected = false;
            BLUSYS_LOGW(kTag, "Wi-Fi disconnected");
            break;
        case CET::time_synced:
            state.time_synced = true;
            BLUSYS_LOGI(kTag, "Time synced (simulated)");
            break;
        case CET::mdns_ready:
            state.mdns_ready = true;
            BLUSYS_LOGI(kTag, "mDNS ready");
            break;
        case CET::connectivity_ready:
            state.conn_capability_ready = true;
            BLUSYS_LOGI(kTag, "connectivity capability ready");
            break;
        case CET::storage_spiffs_mounted:
            state.saw_spiffs = true;
            BLUSYS_LOGI(kTag, "SPIFFS mounted (simulated)");
            break;
        case CET::storage_ready:
            state.storage_ready = true;
            BLUSYS_LOGI(kTag, "storage capability ready");
            break;
        default:
            break;
        }
        break;

    case action_tag::periodic_tick:
        state.tick_count++;
        if (state.tick_count % 10 == 0) {
            BLUSYS_LOGI(kTag, "heartbeat #%d connected=%s",
                        state.tick_count,
                        state.connected ? "yes" : "no");
        }
        break;
    }
}

// ---- on_tick ----

void on_tick(blusys::app::app_ctx &ctx, State & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(Action{.tag = action_tag::periodic_tick});
}

// ---- capability configuration ----

blusys::app::connectivity_capability conn{{
    .wifi_ssid     = "demo-network",
    .wifi_password = "demo-password",
    .sntp_server   = "pool.ntp.org",
    .mdns_hostname = "blusys-headless",
}};

blusys::app::storage_capability stor{{
    .spiffs_base_path = "/fs",
}};

blusys::app::capability_list capabilities{&conn, &stor};

}  // namespace

// ---- app definition ----

static auto spec = blusys::app::app_spec<State, Action>{
    .initial_state  = {},
    .update         = update,
    .on_tick        = on_tick,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms = 1000,
    .capabilities   = &capabilities,
};

BLUSYS_APP_MAIN_HEADLESS(spec)
