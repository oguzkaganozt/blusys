#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"


#include <cstdio>
#include <optional>

namespace headless_telemetry {

namespace {

constexpr const char *kTag = "headless_telemetry";

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
        return op_state::reporting;
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

}  // namespace

const char *op_state_name(op_state s)
{
    switch (s) {
    case op_state::idle:          return "idle";
    case op_state::provisioning:  return "provisioning";
    case op_state::connecting:    return "connecting";
    case op_state::connected:     return "connected";
    case op_state::reporting:     return "reporting";
    case op_state::updating:      return "updating";
    case op_state::error:         return "error";
    }
    return "unknown";
}

std::optional<action> on_event(blusys::event event, app_state &state)
{
    (void)state;
    if (event.source != blusys::event_source::integration) {
        return std::nullopt;
    }

    blusys::capability_event ce{};
    if (!blusys::map_integration_event(event.id, event.kind, &ce)) {
        return std::nullopt;
    }
    ce.payload = event.payload;
    return action{.tag = action_tag::capability_event, .cap_event = ce};
}

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{
    using CET = blusys::capability_event_tag;

    switch (event.tag) {
    case action_tag::capability_event:
        switch (event.cap_event.tag) {
        case CET::wifi_started:
            BLUSYS_LOGI(kTag, "wifi started");
            break;
        case CET::wifi_connected:
            state.wifi_connected = true;
            BLUSYS_LOGI(kTag, "wifi associated");
            break;
        case CET::wifi_connecting:
            BLUSYS_LOGI(kTag, "wifi connecting…");
            break;
        case CET::wifi_got_ip:
            state.has_ip = true;
            if (const auto *conn = ctx.status_of<blusys::connectivity_capability>(); conn != nullptr) {
                BLUSYS_LOGI(kTag, "ip acquired: %s", conn->ip_info.ip);
            }
            break;
        case CET::wifi_disconnected:
            state.wifi_connected = false;
            state.has_ip = false;
            BLUSYS_LOGW(kTag, "wifi disconnected — buffering telemetry");
            break;
        case CET::wifi_reconnecting:
            BLUSYS_LOGI(kTag, "wifi reconnecting...");
            break;
        case CET::time_synced:
            state.time_synced = true;
            BLUSYS_LOGI(kTag, "time synchronized");
            break;
        case CET::time_sync_failed:
            BLUSYS_LOGW(kTag, "SNTP time sync failed (continuing)");
            break;
        case CET::mdns_ready:
            BLUSYS_LOGI(kTag, "mDNS advertising ready");
            break;
        case CET::local_ctrl_ready:
            BLUSYS_LOGI(kTag, "local control endpoint ready");
            break;
        case CET::connectivity_ready:
            state.conn_ready = true;
            BLUSYS_LOGI(kTag, "connectivity capability ready — all services up");
            break;

        case CET::telemetry_delivered:
            if (const auto *tel = ctx.status_of<blusys::telemetry_capability>(); tel != nullptr) {
                state.delivered = tel->total_delivered;
            }
            BLUSYS_LOGI(kTag, "telemetry batch delivered (total=%u)", state.delivered);
            break;
        case CET::telemetry_failed:
            if (const auto *tel = ctx.status_of<blusys::telemetry_capability>(); tel != nullptr) {
                state.delivery_fails = tel->total_failed;
            }
            BLUSYS_LOGW(kTag, "telemetry delivery failed (fails=%u)", state.delivery_fails);
            break;
        case CET::telemetry_buffer_full:
            BLUSYS_LOGW(kTag, "telemetry buffer full — oldest samples dropped");
            break;
        case CET::telemetry_ready:
            BLUSYS_LOGI(kTag, "telemetry capability ready");
            break;
        case CET::telemetry_buffer_flushed:
            BLUSYS_LOGD(kTag, "telemetry buffer flushed to delivery path");
            break;

        case CET::diag_snapshot_ready:
            if (const auto *diag = ctx.fx().diag.status(); diag != nullptr) {
                state.free_heap = diag->last_snapshot.free_heap;
                state.uptime_ms = diag->last_snapshot.uptime_ms;
            }
            break;
        case CET::diagnostics_ready:
            state.diag_ready = true;
            BLUSYS_LOGI(kTag, "diagnostics ready");
            break;
        case CET::diag_console_ready:
            break;

        case CET::ota_check_started:
            break;
        case CET::ota_download_started:
            state.ota_in_progress = true;
            state.ota_progress = 0;
            BLUSYS_LOGI(kTag, "ota: download started");
            break;
        case CET::ota_download_progress:
            state.ota_progress = static_cast<std::uint8_t>(event.cap_event.value);
            BLUSYS_LOGI(kTag, "ota: download %u%%", state.ota_progress);
            break;
        case CET::ota_download_complete:
            BLUSYS_LOGI(kTag, "ota: download complete");
            break;
        case CET::ota_download_failed:
            state.ota_in_progress = false;
            BLUSYS_LOGE(kTag, "ota: download failed — resuming normal operation");
            break;
        case CET::ota_apply_complete:
            state.ota_in_progress = false;
            BLUSYS_LOGI(kTag, "ota: firmware applied — reboot pending");
            break;
        case CET::ota_apply_failed:
            state.ota_in_progress = false;
            BLUSYS_LOGE(kTag, "ota: apply failed — resuming normal operation");
            break;
        case CET::ota_rollback_pending:
            BLUSYS_LOGW(kTag, "ota: rollback pending — will mark valid after verification");
            break;
        case CET::ota_marked_valid:
            BLUSYS_LOGI(kTag, "ota: firmware marked valid");
            break;
        case CET::ota_ready:
            state.ota_ready = true;
            BLUSYS_LOGI(kTag, "ota ready");
            break;

        case CET::prov_started:
            BLUSYS_LOGI(kTag, "provisioning started — awaiting credentials");
            break;
        case CET::prov_credentials_received:
            BLUSYS_LOGI(kTag, "provisioning: credentials received");
            break;
        case CET::prov_success:
            state.provisioned = true;
            BLUSYS_LOGI(kTag, "provisioning succeeded");
            break;
        case CET::prov_failed:
            BLUSYS_LOGE(kTag, "provisioning failed");
            break;
        case CET::prov_already_done:
            state.provisioned = true;
            BLUSYS_LOGI(kTag, "already provisioned — skipping setup");
            break;
        case CET::prov_reset_complete:
            state.provisioned = false;
            BLUSYS_LOGW(kTag, "provisioning credentials cleared");
            break;
        case CET::provisioning_ready:
            BLUSYS_LOGI(kTag, "provisioning capability ready");
            break;

        case CET::storage_spiffs_mounted:
            BLUSYS_LOGI(kTag, "SPIFFS mounted");
            break;
        case CET::storage_fatfs_mounted:
            BLUSYS_LOGI(kTag, "FATFS mounted");
            break;
        case CET::storage_ready:
            state.storage_ready = true;
            BLUSYS_LOGI(kTag, "storage capability ready");
            break;

        default:
            break;
        }
        break;

    case action_tag::sample_tick:
        break;
    }

    refresh_phase(state);
}

}  // namespace headless_telemetry
