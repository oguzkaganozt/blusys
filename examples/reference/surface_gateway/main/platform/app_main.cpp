// Gateway reference — system layer.
//
// Owns capability configuration, event bridge, telemetry recording,
// and platform-conditional entry points.  The gateway runs as an
// interactive device with an operator dashboard by default, proving
// that the same operating model supports optional local UI without
// architecture changes.

#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/framework/platform/auto.hpp"
#include "blusys/framework/platform/auto_shell.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/log.h"
#include "blusys/version.h"

#include <cstdio>
#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

namespace surface_surface_gateway::system {

const char *gateway_profile_label_for_build()
{
    return blusys::app::dashboard_profile_label();
}

const char *gateway_hardware_label_for_build()
{
    return blusys::app::dashboard_hardware_label();
}

const char *gateway_build_version_for_build()
{
#ifdef PROJECT_VER
    return PROJECT_VER;
#elif defined(BLUSYS_APP_BUILD_VERSION)
    return BLUSYS_APP_BUILD_VERSION;
#else
    return BLUSYS_VERSION_STRING;
#endif
}

namespace {

static const blusys::app::view::shell_config kShellConfig =
    blusys::app::auto_shell(blusys::app::auto_profile_dashboard(), "Gateway");

// ---- telemetry delivery callback ----

bool deliver_telemetry(const blusys::app::telemetry_metric *metrics,
                       std::size_t count, void * /*user_ctx*/)
{
    BLUSYS_LOGI("telemetry", "forwarding %u aggregated metrics:", static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count; ++i) {
        BLUSYS_LOGI("telemetry", "  [%u] %s = %.2f  ts=%lu",
                    static_cast<unsigned>(i),
                    metrics[i].name ? metrics[i].name : "?",
                    static_cast<double>(metrics[i].value),
                    static_cast<unsigned long>(metrics[i].timestamp_ms));
    }
    return true;
}

// ---- local control status callback ----

#ifdef ESP_PLATFORM
blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len,
                               size_t *out_len, void * /*user_ctx*/)
{
    int written = std::snprintf(json_buf, buf_len,
        "{\"product\":\"surface-gateway-reference\","
        "\"interface\":\"surface\","
        "\"firmware\":\"%s\","
        "\"profile\":\"%s\"}",
        gateway_build_version_for_build(),
        gateway_profile_label_for_build());
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}
#endif

// ---- capability instances ----

blusys::app::connectivity_config conn_cfg{
#ifdef ESP_PLATFORM
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
#else
    .wifi_ssid     = "host-sim",
    .wifi_password = "",
#endif
    .reconnect_delay_ms = 3000,
    .max_retries        = -1,
    .sntp_server        = "pool.ntp.org",
    .prov_service_name = "blusys-gw",
    .prov_pop          = "gw1234",
    .prov_skip_if_provisioned = true,
};

blusys::app::connectivity_capability connectivity{conn_cfg};

blusys::app::lan_control_capability lan_control{{
    .device_name = "Gateway",
#ifdef ESP_PLATFORM
    .status_cb   = local_ctrl_status,
#endif
    .service_name = "blusys-gw",
    .instance_name = "Blusys Gateway",
}};

blusys::app::telemetry_capability telemetry{{
    .deliver           = deliver_telemetry,
    .flush_threshold   = 12,
    .flush_interval_ms = 20000,
}};

blusys::app::diagnostics_capability diagnostics{{
    .enable_console       = true,
    .snapshot_interval_ms = 3000,
}};

blusys::app::ota_capability ota{{
    .firmware_url    = "https://ota.example.com/gateway.bin",
    .auto_mark_valid = true,
}};

blusys::app::storage_capability storage{{
    .spiffs_base_path = "/gw",
}};

blusys::app::capability_list capabilities{
    &connectivity, &lan_control, &telemetry, &diagnostics, &ota, &storage};

// ---- sensor helpers ----

float drift_sensor(float base, float range, std::uint32_t tick)
{
    float phase = static_cast<float>(tick % 180) / 180.0f;
    float offset = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
    return base + (offset - 0.5f) * range;
}

// ---- on_tick: simulated downstream aggregation and telemetry ----

void on_tick(blusys::app::app_ctx & /*ctx*/, blusys::app::app_services &svc, app_state &state, std::uint32_t now_ms)
{
    (void)svc;
    ++state.sample_count;

    // Simulate downstream device activity.
    state.active_devices = 1 + static_cast<std::int32_t>(state.sample_count % 3);
    state.agg_throughput = drift_sensor(12.0f, 8.0f, state.sample_count);

    // Record aggregated telemetry when operational.
    if (state.phase == op_state::operational) {
        telemetry.record("active_devices",
                         static_cast<float>(state.active_devices), now_ms);
        telemetry.record("throughput", state.agg_throughput, now_ms);
    }

    // Periodic heartbeat log.
    if (state.sample_count % 30 == 0) {
        BLUSYS_LOGI("surface_gateway", "heartbeat #%lu  phase=%s  dev=%ld  tput=%.1f  "
                                "delivered=%u fails=%u",
                    static_cast<unsigned long>(state.sample_count),
                    op_state_name(state.phase),
                    static_cast<long>(state.active_devices),
                    static_cast<double>(state.agg_throughput),
                    state.delivered, state.delivery_fails);
    }
}

// ---- app spec ----

const blusys::app::app_spec<app_state, action> spec{
    .initial_state  = {},
    .update         = update,
    .on_init        = ui::on_init,
    .on_tick        = on_tick,
    .map_intent     = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms = 300,
    .capabilities   = &capabilities,
    .theme          = &blusys::app::presets::operational_light(),
    .shell          = &kShellConfig,
};

}  // namespace

}  // namespace surface_surface_gateway::system

BLUSYS_APP_DASHBOARD(surface_gateway::system::spec, "Blusys Gateway")
