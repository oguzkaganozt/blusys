#include "core/app_logic.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/view/bindings.hpp"
#include "blusys/app/view/composites.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#endif

#include "blusys/log.h"

#include <cstdio>

namespace gateway {

namespace {

constexpr const char *kTag = "gateway";

op_state compute_phase(const app_state &state)
{
    if (state.ota_in_progress) {
        return op_state::updating;
    }
    if (!state.provisioned && !state.conn_ready) {
        return op_state::provisioning;
    }
    if (!state.has_ip) {
        return op_state::connecting;
    }
    if (state.has_ip && state.time_synced && state.conn_ready) {
        return op_state::operational;
    }
    if (state.has_ip) {
        return op_state::connected;
    }
    return op_state::idle;
}

void refresh_phase(app_state &state)
{
    op_state next = compute_phase(state);
    if (next != state.phase) {
        BLUSYS_LOGI(kTag, "phase: %s -> %s",
                    op_state_name(state.phase), op_state_name(next));
        state.phase = next;
    }
}

#ifdef BLUSYS_FRAMEWORK_HAS_UI
void sync_shell(app_state &state)
{
    if (state.shell_badge != nullptr) {
        const bool ready = (state.phase == op_state::operational);
        blusys::app::view::set_badge_text(state.shell_badge,
                                          ready ? "Online" : op_state_name(state.phase));
        blusys::app::view::set_badge_level(
            state.shell_badge,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }

    if (state.shell_detail != nullptr) {
        char detail[48];
        std::snprintf(detail, sizeof(detail), "%ld dev  %.0f msg/s",
                      static_cast<long>(state.active_devices),
                      static_cast<double>(state.agg_throughput));
        blusys::app::view::set_text(state.shell_detail, detail);
    }
}

void sync_dashboard(app_state &state)
{
    char v_devices[32];
    char v_tput[16];
    const char *v_uplink = state.conn_ready ? "Connected" : "Offline";
    const char *v_storage = state.storage_ready ? "Mounted" : "Pending";

    std::snprintf(v_devices, sizeof(v_devices), "%ld / %ld",
                  static_cast<long>(state.active_devices),
                  static_cast<long>(state.device_count));
    std::snprintf(v_tput, sizeof(v_tput), "%.1f msg/s",
                  static_cast<double>(state.agg_throughput));

    const blusys::app::view::key_value_quad row{
        {state.dash_devices, state.dash_throughput, state.dash_uplink, state.dash_storage},
    };
    blusys::app::view::sync_key_value_quad(row, v_devices, v_tput, v_uplink, v_storage);
}

void sync_status(blusys::app::app_ctx &ctx, app_state &state)
{
    blusys::app::screens::status_screen_update(state.status_handles, ctx);
}

void sync_all(blusys::app::app_ctx &ctx, app_state &state)
{
    sync_shell(state);
    sync_dashboard(state);
    sync_status(ctx, state);
}
#endif  // BLUSYS_FRAMEWORK_HAS_UI

}  // namespace

const char *op_state_name(op_state s)
{
    switch (s) {
    case op_state::idle:          return "idle";
    case op_state::provisioning:  return "provisioning";
    case op_state::connecting:    return "connecting";
    case op_state::connected:     return "connected";
    case op_state::operational:   return "operational";
    case op_state::updating:      return "updating";
    case op_state::error:         return "error";
    }
    return "unknown";
}

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    switch (event.tag) {

    // ---- connectivity ----

    case action_tag::wifi_started:
        BLUSYS_LOGI(kTag, "wifi stack started");
        break;

    case action_tag::wifi_connected:
        state.wifi_connected = true;
        BLUSYS_LOGI(kTag, "wifi associated");
        break;

    case action_tag::wifi_connecting:
        BLUSYS_LOGI(kTag, "wifi connecting…");
        break;

    case action_tag::wifi_got_ip:
        state.has_ip = true;
        if (const auto *conn = ctx.connectivity(); conn != nullptr) {
            BLUSYS_LOGI(kTag, "uplink acquired: %s", conn->ip_info.ip);
        }
        break;

    case action_tag::wifi_disconnected:
        state.wifi_connected = false;
        state.has_ip = false;
        BLUSYS_LOGW(kTag, "uplink lost — buffering downstream telemetry");
        break;

    case action_tag::wifi_reconnecting:
        BLUSYS_LOGI(kTag, "uplink reconnecting...");
        break;

    case action_tag::time_synced:
        state.time_synced = true;
        BLUSYS_LOGI(kTag, "time synchronized");
        break;

    case action_tag::time_sync_failed:
        BLUSYS_LOGW(kTag, "SNTP time sync failed (continuing)");
        break;

    case action_tag::mdns_ready:
        BLUSYS_LOGI(kTag, "mDNS ready");
        break;

    case action_tag::local_ctrl_ready:
        BLUSYS_LOGI(kTag, "local control ready");
        break;

    case action_tag::conn_capability_ready:
        state.conn_ready = true;
        state.wifi_connected = true;
        BLUSYS_LOGI(kTag, "connectivity capability ready — all services up");
        break;

    // ---- telemetry ----

    case action_tag::telemetry_delivered:
        if (const auto *tel = ctx.telemetry(); tel != nullptr) {
            state.delivered = tel->total_delivered;
        }
        BLUSYS_LOGI(kTag, "aggregated telemetry delivered (total=%u)", state.delivered);
        break;

    case action_tag::telemetry_failed:
        if (const auto *tel = ctx.telemetry(); tel != nullptr) {
            state.delivery_fails = tel->total_failed;
        }
        BLUSYS_LOGW(kTag, "telemetry delivery failed (fails=%u)", state.delivery_fails);
        break;

    case action_tag::telemetry_buffer_full:
        BLUSYS_LOGW(kTag, "telemetry buffer full");
        break;

    case action_tag::telemetry_capability_ready:
        BLUSYS_LOGI(kTag, "telemetry capability ready");
        break;

    case action_tag::telemetry_buffer_flushed:
        BLUSYS_LOGD(kTag, "telemetry buffer flushed");
        break;

    // ---- diagnostics ----

    case action_tag::diag_snapshot:
        if (const auto *diag = ctx.diagnostics(); diag != nullptr) {
            state.diagnostics = *diag;
        }
        break;

    case action_tag::diag_capability_ready:
        state.diag_ready = true;
        BLUSYS_LOGI(kTag, "diagnostics ready");
        break;

    // ---- ota ----

    case action_tag::ota_download_started:
        state.ota_in_progress = true;
        state.ota_progress = 0;
        BLUSYS_LOGI(kTag, "ota: download started");
        break;

    case action_tag::ota_download_progress:
        state.ota_in_progress = true;
        state.ota_progress = static_cast<std::uint8_t>(event.value);
        BLUSYS_LOGI(kTag, "ota: %u%%", state.ota_progress);
        break;

    case action_tag::ota_download_complete:
        BLUSYS_LOGI(kTag, "ota: download complete");
        break;

    case action_tag::ota_download_failed:
        state.ota_in_progress = false;
        BLUSYS_LOGE(kTag, "ota: download failed");
        break;

    case action_tag::ota_apply_complete:
        state.ota_in_progress = false;
        BLUSYS_LOGI(kTag, "ota: firmware applied — reboot pending");
        break;

    case action_tag::ota_apply_failed:
        state.ota_in_progress = false;
        BLUSYS_LOGE(kTag, "ota: apply failed — resuming normal operation");
        break;

    case action_tag::ota_rollback_pending:
        BLUSYS_LOGW(kTag, "ota: rollback pending");
        break;

    case action_tag::ota_marked_valid:
        BLUSYS_LOGI(kTag, "ota: firmware marked valid");
        break;

    case action_tag::ota_capability_ready:
        state.ota_ready = true;
        BLUSYS_LOGI(kTag, "ota ready");
        break;

    // ---- provisioning ----

    case action_tag::prov_started:
        BLUSYS_LOGI(kTag, "provisioning started");
        break;

    case action_tag::prov_credentials_received:
        BLUSYS_LOGI(kTag, "provisioning: credentials received");
        break;

    case action_tag::prov_success:
    case action_tag::prov_already_done:
        state.provisioned = true;
        break;

    case action_tag::prov_capability_ready:
        BLUSYS_LOGI(kTag, "provisioning capability ready");
        break;

    case action_tag::prov_failed:
        BLUSYS_LOGE(kTag, "provisioning failed");
        break;

    case action_tag::prov_reset_complete:
        state.provisioned = false;
        BLUSYS_LOGW(kTag, "provisioning reset");
        break;

    // ---- storage ----

    case action_tag::storage_spiffs_mounted:
        BLUSYS_LOGI(kTag, "SPIFFS mounted");
        break;

    case action_tag::storage_fatfs_mounted:
        BLUSYS_LOGI(kTag, "FATFS mounted");
        break;

    case action_tag::storage_capability_ready:
        state.storage_ready = true;
        BLUSYS_LOGI(kTag, "storage capability ready");
        break;

    // ---- navigation (interactive) ----

    case action_tag::show_dashboard:
        ctx.navigate_to(route_dashboard);
        break;

    case action_tag::show_status:
        ctx.navigate_to(route_status);
        break;

    case action_tag::show_settings:
        ctx.navigate_to(route_settings);
        break;

    case action_tag::open_about:
        ctx.navigate_push(route_about);
        break;

    // ---- periodic ----

    case action_tag::sample_tick:
        break;
    }

    refresh_phase(state);

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    sync_all(ctx, state);
#endif
}

bool map_intent(blusys::framework::intent intent, action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = action{.tag = action_tag::show_status};
        return true;
    case blusys::framework::intent::decrement:
        *out = action{.tag = action_tag::show_dashboard};
        return true;
    case blusys::framework::intent::confirm:
        *out = action{.tag = action_tag::show_settings};
        return true;
    case blusys::framework::intent::cancel:
        *out = action{.tag = action_tag::show_dashboard};
        return true;
    default:
        return false;
    }
}

}  // namespace gateway
