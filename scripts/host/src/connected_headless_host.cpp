// connected_headless_host — connected headless product running on host.
//
// Same reducer logic as the device connected_headless example, with
// simulated connectivity and storage capabilities. The host stubs post
// fake wifi_got_ip / time_synced / capability_ready events on the first
// frame so the full app event flow exercises without hardware.

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/log.h"

#include <cstdint>

namespace {

constexpr const char *kTag = "app";

// ---- state ----

struct State {
    bool connected    = false;
    bool time_synced  = false;
    bool capability_ready = false;
    int  tick_count   = 0;
};

// ---- actions ----

enum class Action {
    wifi_got_ip,
    wifi_disconnected,
    time_synced,
    capability_ready,
    periodic_tick,
};

// ---- reducer ----

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    switch (action) {
    case Action::wifi_got_ip:
        state.connected = true;
        if (auto *conn = ctx.connectivity(); conn != nullptr) {
            BLUSYS_LOGI(kTag, "Wi-Fi connected, IP: %s", conn->ip_info.ip);
        }
        break;

    case Action::wifi_disconnected:
        state.connected = false;
        BLUSYS_LOGW(kTag, "Wi-Fi disconnected");
        break;

    case Action::time_synced:
        state.time_synced = true;
        BLUSYS_LOGI(kTag, "Time synced (simulated)");
        break;

    case Action::capability_ready:
        state.capability_ready = true;
        BLUSYS_LOGI(kTag, "All services ready — product operational");
        break;

    case Action::periodic_tick:
        state.tick_count++;
        if (state.tick_count % 10 == 0) {
            BLUSYS_LOGI(kTag, "heartbeat #%d connected=%s",
                        state.tick_count,
                        state.connected ? "yes" : "no");
        }
        break;
    }
}

// ---- event bridge ----

bool map_event(std::uint32_t id, std::uint32_t /*code*/,
               const void * /*payload*/, Action *out)
{
    using CE = blusys::app::connectivity_event;
    switch (static_cast<CE>(id)) {
    case CE::wifi_got_ip:       *out = Action::wifi_got_ip;      return true;
    case CE::wifi_disconnected: *out = Action::wifi_disconnected; return true;
    case CE::time_synced:       *out = Action::time_synced;      return true;
    case CE::capability_ready:      *out = Action::capability_ready;     return true;
    default: return false;
    }
}

// ---- on_tick ----

void on_tick(blusys::app::app_ctx &ctx, State & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(Action::periodic_tick);
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
    .map_event      = map_event,
    .tick_period_ms = 1000,
    .capabilities   = &capabilities,
};

BLUSYS_APP_MAIN_HEADLESS(spec)
