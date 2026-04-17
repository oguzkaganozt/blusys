#include "core/app_logic.hpp"

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/hal/log.h"
#include "blusys/hal/version.h"

#include <cstdio>
#include <cstdint>

namespace surface_gateway::system {

namespace {

[[maybe_unused]] const char *gateway_build_version_for_build()
{
#ifdef PROJECT_VER
    return PROJECT_VER;
#elif defined(BLUSYS_APP_BUILD_VERSION)
    return BLUSYS_APP_BUILD_VERSION;
#else
    return BLUSYS_VERSION_STRING;
#endif
}

bool deliver_telemetry(const blusys::telemetry_metric *metrics,
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

#ifdef ESP_PLATFORM
blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len,
                               size_t *out_len, void * /*user_ctx*/)
{
    int written = std::snprintf(json_buf, buf_len,
        "{\"product\":\"surface-gateway-headless\","
        "\"interface\":\"headless\","
        "\"firmware\":\"%s\"}",
        gateway_build_version_for_build());
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}
#endif

blusys::connectivity_config conn_cfg{
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

blusys::connectivity_capability connectivity{conn_cfg};
blusys::lan_control_capability lan_control{{
    .device_name = "Gateway",
#ifdef ESP_PLATFORM
    .status_cb   = local_ctrl_status,
#endif
    .service_name = "blusys-gw",
    .instance_name = "Blusys Gateway",
}};
blusys::telemetry_capability telemetry{{
    .deliver           = deliver_telemetry,
    .flush_threshold   = 12,
    .flush_interval_ms = 20000,
}};
blusys::diagnostics_capability diagnostics{{
    .enable_console       = true,
    .snapshot_interval_ms = 3000,
}};
blusys::ota_capability ota{{
    .firmware_url    = "https://ota.example.com/gateway.bin",
    .auto_mark_valid = true,
}};
blusys::storage_capability storage{{
    .spiffs_base_path = "/gw",
}};

blusys::capability_list_storage capabilities{
    &connectivity, &lan_control, &telemetry, &diagnostics, &ota, &storage};

void on_tick(blusys::app_ctx & /*ctx*/, blusys::app_fx &fx, app_state &state, std::uint32_t now_ms)
{
    (void)fx;
    ++state.sample_count;
    state.active_devices = 1 + static_cast<std::int32_t>(state.sample_count % 3);
    state.agg_throughput = 12.0f + static_cast<float>(state.sample_count % 8);

    if (state.phase == op_state::operational) {
        telemetry.record("active_devices", static_cast<float>(state.active_devices), now_ms);
        telemetry.record("throughput", state.agg_throughput, now_ms);
    }

    if (state.sample_count % 30 == 0) {
        BLUSYS_LOGI("surface_gateway", "heartbeat #%lu  phase=%s  dev=%ld  tput=%.1f  delivered=%u fails=%u",
                    static_cast<unsigned long>(state.sample_count),
                    op_state_name(state.phase),
                    static_cast<long>(state.active_devices),
                    static_cast<double>(state.agg_throughput),
                    state.delivered, state.delivery_fails);
    }
}

const blusys::app_spec<surface_gateway::app_state, surface_gateway::action> spec{
    .initial_state  = {},
    .update         = surface_gateway::update,
    .on_tick        = on_tick,
    .on_event       = surface_gateway::on_event,
    .tick_period_ms = 300,
    .capabilities   = &capabilities,
};

}  // namespace

}  // namespace surface_gateway::system

BLUSYS_APP_MAIN_HEADLESS(surface_gateway::system::spec)
