// Gateway reference — system layer.
//
// Owns capability configuration, event bridge, telemetry recording,
// and platform-conditional entry points.  The gateway runs as an
// interactive device with an operator dashboard by default, proving
// that the same operating model supports optional local UI without
// architecture changes.

#include "logic/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/layout_surface.hpp"
#include "blusys/app/theme_presets.hpp"
#include "blusys/log.h"

#include <cstdio>
#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/app/profiles/ili9341.hpp"
#include "blusys/app/profiles/ili9488.hpp"
#endif

namespace gateway::system {

blusys::app::device_profile gateway_device_profile_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_GW_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_GW_DISPLAY_PROFILE_ILI9488
    return blusys::app::profiles::ili9488_480x320();
#elif defined(ESP_PLATFORM)
    return blusys::app::profiles::ili9341_320x240();
#else
    blusys::app::device_profile p{};
#if defined(BLUSYS_GW_HOST_DISPLAY_PROFILE) && (BLUSYS_GW_HOST_DISPLAY_PROFILE == 1)
    p.lcd.width          = 480;
    p.lcd.height         = 320;
#else
    p.lcd.width          = 320;
    p.lcd.height         = 240;
#endif
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_UI_PANEL_KIND_RGB565;
    return p;
#endif
}

namespace {

const blusys::app::view::shell_config gateway_shell_for_profile()
{
    const auto prof = gateway_device_profile_for_build();
    const auto h    = blusys::app::layout::classify(prof);
    blusys::app::view::shell_config c{};
    c.header.enabled = true;
    c.header.title   = "Gateway";
    c.status.enabled = h.shell != blusys::app::layout::shell_density::minimal;
    c.tabs.enabled =
        h.size_class != blusys::app::layout::surface_size::tiny_mono;
    return c;
}

static const blusys::app::view::shell_config kShellConfig = gateway_shell_for_profile();

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
        "{\"product\":\"gateway-reference\","
        "\"archetype\":\"gateway/controller\","
        "\"firmware\":\"phase6-preview\"}");
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
    .mdns_hostname      = "blusys-gw",
    .mdns_instance_name = "Blusys Gateway",
    .local_ctrl_device_name = "Gateway",
#ifdef ESP_PLATFORM
    .local_ctrl_status_cb   = local_ctrl_status,
#endif
};

blusys::app::connectivity_capability connectivity{conn_cfg};

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

blusys::app::provisioning_capability provisioning{{
    .service_name        = "blusys-gw",
    .pop                 = "gw1234",
    .skip_if_provisioned = true,
}};

blusys::app::storage_capability storage{{
    .spiffs_base_path = "/gw",
}};

blusys::app::capability_list capabilities{
    &connectivity, &telemetry, &diagnostics, &ota, &provisioning, &storage};

// ---- sensor helpers ----

float drift_sensor(float base, float range, std::uint32_t tick)
{
    float phase = static_cast<float>(tick % 180) / 180.0f;
    float offset = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
    return base + (offset - 0.5f) * range;
}

// ---- on_tick: simulated downstream aggregation and telemetry ----

void on_tick(blusys::app::app_ctx & /*ctx*/, app_state &state, std::uint32_t now_ms)
{
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
        BLUSYS_LOGI("gateway", "heartbeat #%lu  phase=%s  dev=%ld  tput=%.1f  "
                                "delivered=%u fails=%u",
                    static_cast<unsigned long>(state.sample_count),
                    op_state_name(state.phase),
                    static_cast<long>(state.active_devices),
                    static_cast<double>(state.agg_throughput),
                    state.delivered, state.delivery_fails);
    }
}

// ---- event bridge ----

bool map_event(std::uint32_t id, std::uint32_t code,
               const void * /*payload*/, action *out)
{
    // Connectivity events
    using CE = blusys::app::connectivity_event;
    switch (static_cast<CE>(id)) {
    case CE::wifi_got_ip:       *out = action{.tag = action_tag::wifi_got_ip};       return true;
    case CE::wifi_disconnected: *out = action{.tag = action_tag::wifi_disconnected}; return true;
    case CE::wifi_reconnecting: *out = action{.tag = action_tag::wifi_reconnecting}; return true;
    case CE::time_synced:       *out = action{.tag = action_tag::time_synced};       return true;
    case CE::bundle_ready:      *out = action{.tag = action_tag::conn_bundle_ready}; return true;
    default: break;
    }

    // Telemetry events
    using TE = blusys::app::telemetry_event;
    switch (static_cast<TE>(id)) {
    case TE::delivery_ok:     *out = action{.tag = action_tag::telemetry_delivered}; return true;
    case TE::delivery_failed: *out = action{.tag = action_tag::telemetry_failed};    return true;
    default: break;
    }

    // Diagnostics events
    using DE = blusys::app::diagnostics_event;
    switch (static_cast<DE>(id)) {
    case DE::snapshot_ready: *out = action{.tag = action_tag::diag_snapshot};     return true;
    case DE::bundle_ready:   *out = action{.tag = action_tag::diag_bundle_ready}; return true;
    default: break;
    }

    // OTA events
    using OE = blusys::app::ota_event;
    switch (static_cast<OE>(id)) {
    case OE::download_progress: *out = action{.tag = action_tag::ota_download_progress,
                                               .value = static_cast<std::int32_t>(code)}; return true;
    case OE::apply_complete:    *out = action{.tag = action_tag::ota_apply_complete};    return true;
    case OE::apply_failed:      *out = action{.tag = action_tag::ota_apply_failed};      return true;
    case OE::bundle_ready:      *out = action{.tag = action_tag::ota_bundle_ready};      return true;
    default: break;
    }

    // Provisioning events
    using PE = blusys::app::provisioning_event;
    switch (static_cast<PE>(id)) {
    case PE::success:             *out = action{.tag = action_tag::prov_success};      return true;
    case PE::already_provisioned: *out = action{.tag = action_tag::prov_already_done}; return true;
    case PE::bundle_ready:        *out = action{.tag = action_tag::prov_bundle_ready}; return true;
    default: break;
    }

    // Storage events
    using SE = blusys::app::storage_event;
    switch (static_cast<SE>(id)) {
    case SE::bundle_ready: *out = action{.tag = action_tag::storage_bundle_ready}; return true;
    default: break;
    }

    return false;
}

// ---- app spec ----

const blusys::app::app_spec<app_state, action> spec{
    .initial_state  = {},
    .update         = update,
    .on_init        = ui::on_init,
    .on_tick        = on_tick,
    .map_intent     = map_intent,
    .map_event      = map_event,
    .tick_period_ms = 300,
    .capabilities   = &capabilities,
    .theme          = &blusys::app::presets::operational_light(),
    .shell          = &kShellConfig,
};

}  // namespace

}  // namespace gateway::system

// ---- platform entry ----

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(gateway::system::spec,
                       gateway::system::gateway_device_profile_for_build())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(
    gateway::system::spec,
    (blusys::app::host_profile{
#if defined(BLUSYS_GW_HOST_DISPLAY_PROFILE) && (BLUSYS_GW_HOST_DISPLAY_PROFILE == 1)
        .hor_res = 480,
        .ver_res = 320,
#else
        .hor_res = 320,
        .ver_res = 240,
#endif
        .title = "Blusys Gateway",
    }))
#endif
