// Connected headless product example — blusys::app with capabilities.
//
// Demonstrates the v7 product path for a headless connected device:
//   - connectivity capability handles Wi-Fi, SNTP, mDNS, and local control
//   - storage capability mounts SPIFFS for persistent data
//   - app code only declares config and reacts to events through the reducer

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/log.h"

#include <cstdio>
#include <cstring>
#include <ctime>

static const char *TAG = "app";

// ---- app state ----

struct State {
    bool connected      = false;
    bool time_synced    = false;
    bool capability_ready   = false;
    int  tick_count     = 0;
};

// ---- app actions ----

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

// ---- on_tick: periodic heartbeat ----

void on_tick(blusys::app::app_ctx &ctx, State & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(Action::periodic_tick);
}

// ---- local control status callback ----

static blusys_err_t status_cb(char *json_buf, size_t buf_len,
                               size_t *out_len, void * /*user_ctx*/)
{
    int written = std::snprintf(json_buf, buf_len,
                                 "{\"product\":\"connected-headless-demo\"}");
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}

// ---- capability configuration ----

static blusys::app::connectivity_capability conn{{
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,

    .sntp_server   = "pool.ntp.org",

    .mdns_hostname      = "blusys-headless",
    .mdns_instance_name = "Blusys Headless Demo",

    .local_ctrl_device_name = "Blusys Headless",
    .local_ctrl_status_cb   = status_cb,
}};

static blusys::app::storage_capability stor{{
    .spiffs_base_path = "/fs",
}};

static blusys::app::capability_list capabilities{&conn, &stor};

// ---- app spec ----

static auto spec = blusys::app::app_spec<State, Action>{
    .initial_state = {},
    .update        = update,
    .on_tick       = on_tick,
    .map_event     = map_event,
    .tick_period_ms = 100,
    .capabilities  = &capabilities,
};

BLUSYS_APP_MAIN_HEADLESS(spec)
