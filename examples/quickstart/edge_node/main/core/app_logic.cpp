#include "core/app_logic.hpp"

#include "blusys/log.h"

#include <cstdio>

namespace edge_node {

namespace {

constexpr const char *kTag = "edge_node";

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

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    switch (event.tag) {

    // ---- connectivity ----

    case action_tag::wifi_started:
        BLUSYS_LOGI(kTag, "wifi started");
        break;

    case action_tag::wifi_connected:
        state.wifi_connected = true;
        BLUSYS_LOGI(kTag, "wifi associated");
        break;

    case action_tag::wifi_got_ip:
        state.has_ip = true;
        if (const auto *conn = ctx.connectivity(); conn != nullptr) {
            BLUSYS_LOGI(kTag, "ip acquired: %s", conn->ip_info.ip);
        }
        break;

    case action_tag::wifi_disconnected:
        state.wifi_connected = false;
        state.has_ip = false;
        BLUSYS_LOGW(kTag, "wifi disconnected — buffering telemetry");
        break;

    case action_tag::wifi_reconnecting:
        BLUSYS_LOGI(kTag, "wifi reconnecting...");
        break;

    case action_tag::time_synced:
        state.time_synced = true;
        BLUSYS_LOGI(kTag, "time synchronized");
        break;

    case action_tag::conn_capability_ready:
        state.conn_ready = true;
        BLUSYS_LOGI(kTag, "connectivity capability ready — all services up");
        break;

    // ---- telemetry ----

    case action_tag::telemetry_delivered:
        if (const auto *tel = ctx.telemetry(); tel != nullptr) {
            state.delivered = tel->total_delivered;
        }
        BLUSYS_LOGI(kTag, "telemetry batch delivered (total=%u)", state.delivered);
        break;

    case action_tag::telemetry_failed:
        if (const auto *tel = ctx.telemetry(); tel != nullptr) {
            state.delivery_fails = tel->total_failed;
        }
        BLUSYS_LOGW(kTag, "telemetry delivery failed (fails=%u)", state.delivery_fails);
        break;

    case action_tag::telemetry_buffer_full:
        BLUSYS_LOGW(kTag, "telemetry buffer full — oldest samples dropped");
        break;

    // ---- diagnostics ----

    case action_tag::diag_snapshot:
        if (const auto *diag = ctx.diagnostics(); diag != nullptr) {
            state.free_heap = diag->last_snapshot.free_heap;
            state.uptime_ms = diag->last_snapshot.uptime_ms;
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
        state.ota_progress = static_cast<std::uint8_t>(event.value);
        BLUSYS_LOGI(kTag, "ota: download %u%%", state.ota_progress);
        break;

    case action_tag::ota_download_complete:
        BLUSYS_LOGI(kTag, "ota: download complete");
        break;

    case action_tag::ota_download_failed:
        state.ota_in_progress = false;
        BLUSYS_LOGE(kTag, "ota: download failed — resuming normal operation");
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
        BLUSYS_LOGW(kTag, "ota: rollback pending — will mark valid after verification");
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
        BLUSYS_LOGI(kTag, "provisioning started — awaiting credentials");
        break;

    case action_tag::prov_success:
        state.provisioned = true;
        BLUSYS_LOGI(kTag, "provisioning succeeded");
        break;

    case action_tag::prov_failed:
        BLUSYS_LOGE(kTag, "provisioning failed");
        break;

    case action_tag::prov_already_done:
        state.provisioned = true;
        BLUSYS_LOGI(kTag, "already provisioned — skipping setup");
        break;

    case action_tag::prov_capability_ready:
        state.provisioned = true;
        BLUSYS_LOGI(kTag, "provisioning capability ready");
        break;

    // ---- storage ----

    case action_tag::storage_capability_ready:
        state.storage_ready = true;
        BLUSYS_LOGI(kTag, "storage mounted");
        break;

    // ---- periodic tick ----

    case action_tag::sample_tick:
        break;  // handled in on_tick
    }

    refresh_phase(state);
}

}  // namespace edge_node
