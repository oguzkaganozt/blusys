// Edge node reference — system layer.
//
// Owns capability configuration, event bridge, telemetry recording,
// and the headless or optional local mono UI entry point.  Product
// behavior stays in core/.

#include "core/app_logic.hpp"

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/build_profile.hpp"
#include "blusys/log.h"

#include <cstdio>
#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "ui/app_ui.hpp"
#endif

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && defined(ESP_PLATFORM) && \
    defined(CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI) && CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI
#include "blusys/app/theme_presets.hpp"
#endif

namespace edge_node::system {

namespace {

#ifdef ESP_PLATFORM
const char *edge_surface_label_for_build()
{
#if defined(BLUSYS_FRAMEWORK_HAS_UI) && defined(ESP_PLATFORM) && \
    defined(CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI) && CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI
    return "SSD1306 128x64";
#else
    return "headless";
#endif
}
#endif

#ifndef BLUSYS_FRAMEWORK_HAS_UI
void on_init_host(blusys::app::app_ctx & /*ctx*/, app_state & /*state*/)
{
    BLUSYS_LOGI("edge_node", "edge node initialized — entering operational loop");
}
#endif

// ---- telemetry delivery callback ----

bool deliver_telemetry(const blusys::app::telemetry_metric *metrics,
                       std::size_t count, void * /*user_ctx*/)
{
    // In a real product this would send metrics over MQTT, HTTP, or a
    // custom protocol.  For the reference archetype we log each batch
    // to demonstrate the delivery contract.
    BLUSYS_LOGI("telemetry", "delivering %u metrics:", static_cast<unsigned>(count));
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
        "{\"product\":\"edge-node-reference\","
        "\"archetype\":\"edge_node\","
        "\"firmware\":\"%s\","
        "\"surface\":\"%s\"}",
        blusys::app::build_profile::version_string(),
        edge_surface_label_for_build());
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
    .reconnect_delay_ms = 5000,
    .max_retries        = 10,
    .sntp_server        = "pool.ntp.org",
    .mdns_hostname      = "blusys-edge",
    .mdns_instance_name = "Blusys Edge Node",
    .local_ctrl_device_name = "Edge Node",
#ifdef ESP_PLATFORM
    .local_ctrl_status_cb   = local_ctrl_status,
#endif
};

blusys::app::connectivity_capability connectivity{conn_cfg};

blusys::app::telemetry_capability telemetry{{
    .deliver           = deliver_telemetry,
    .flush_threshold   = 8,
    .flush_interval_ms = 15000,
}};

blusys::app::diagnostics_capability diagnostics{{
    .enable_console       = true,
    .snapshot_interval_ms = 5000,
}};

blusys::app::ota_capability ota{{
    .firmware_url    = "https://ota.example.com/edge-node.bin",
    .auto_mark_valid = true,
}};

blusys::app::provisioning_capability provisioning{{
    .service_name        = "edge-node",
    .pop                 = "edge1234",
    .skip_if_provisioned = true,
}};

blusys::app::storage_capability storage{{
    .spiffs_base_path = "/edge",
}};

blusys::app::capability_list capabilities{
    &connectivity, &telemetry, &diagnostics, &ota, &provisioning, &storage};

// ---- sensor helpers ----

float drift_sensor(float base, float range, std::uint32_t tick)
{
    float phase = static_cast<float>(tick % 120) / 120.0f;
    float offset = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
    return base + (offset - 0.5f) * range;
}

// ---- on_tick: sensor sampling and telemetry recording ----
// Lives in integration/ because it needs direct access to the telemetry
// capability instance for record().

void on_tick(blusys::app::app_ctx & /*ctx*/, app_state &state, std::uint32_t now_ms)
{
    ++state.sample_count;

    // Simulate sensor readings.
    state.temperature = drift_sensor(22.5f, 6.0f, state.sample_count);
    state.humidity    = drift_sensor(48.0f, 20.0f, state.sample_count + 40);

    // Record telemetry when in reporting phase.
    if (state.phase == op_state::reporting) {
        telemetry.record("temperature", state.temperature, now_ms);
        telemetry.record("humidity", state.humidity, now_ms);
    }

    // Periodic heartbeat log.
    if (state.sample_count % 50 == 0) {
        BLUSYS_LOGI("edge_node", "heartbeat #%lu  phase=%s  temp=%.1f  hum=%.1f  "
                                  "delivered=%u fails=%u heap=%lu",
                    static_cast<unsigned long>(state.sample_count),
                    op_state_name(state.phase),
                    static_cast<double>(state.temperature),
                    static_cast<double>(state.humidity),
                    state.delivered, state.delivery_fails,
                    static_cast<unsigned long>(state.free_heap));
    }
}

// ---- app spec ----

const blusys::app::app_spec<app_state, action> spec{
    .initial_state  = {},
    .update         = update,
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    .on_init        = edge_node::ui::on_init,
#else
    .on_init        = on_init_host,
#endif
    .on_tick        = on_tick,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms = 200,
    .capabilities   = &capabilities,
#ifdef BLUSYS_FRAMEWORK_HAS_UI
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI) && \
    CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI
    .theme          = &blusys::app::presets::oled(),
#endif
#endif
};

}  // namespace

}  // namespace edge_node::system

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI) && \
    CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI && defined(BLUSYS_FRAMEWORK_HAS_UI)
#include "blusys/app/profiles/ssd1306.hpp"
BLUSYS_APP_MAIN_DEVICE(edge_node::system::spec,
                       blusys::app::profiles::ssd1306_128x64())
#else
BLUSYS_APP_MAIN_HEADLESS(edge_node::system::spec)
#endif
