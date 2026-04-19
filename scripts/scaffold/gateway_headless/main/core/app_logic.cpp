#include "core/app_logic.hpp"

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/hal/log.h"

#include <optional>

namespace surface_gateway {

namespace {

constexpr const char *kTag = "surface_gateway";

void refresh_phase(app_state &state)
{
    const bool ready = state.connectivity_ready
                    && state.storage_ready
                    && state.diagnostics_ready
                    && state.telemetry_ready
                    && state.ota_ready
                    && state.lan_control_ready;

    const op_state next = ready ? op_state::operational : op_state::idle;
    if (next != state.phase) {
        BLUSYS_LOGI(kTag, "phase: %s -> %s", op_state_name(state.phase), op_state_name(next));
        state.phase = next;
    }
}

}  // namespace

const char *op_state_name(op_state state)
{
    switch (state) {
    case op_state::idle:        return "idle";
    case op_state::operational: return "operational";
    }
    return "?";
}

std::optional<action> on_event(blusys::event event, app_state &state)
{
    (void)state;
    if (event.source != blusys::event_source::integration) {
        return std::nullopt;
    }

    const auto *ce = blusys::event_capability(event);
    if (ce == nullptr) {
        return std::nullopt;
    }
    return action{.tag = action_tag::capability_event, .cap_event = *ce};
}

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{
    using CET = blusys::capability_event_tag;

    if (event.tag != action_tag::capability_event) {
        return;
    }

    switch (event.cap_event.tag) {
    case CET::wifi_started:
    case CET::wifi_connecting:
    case CET::time_synced:
    case CET::time_sync_failed:
    case CET::mdns_ready:
    case CET::local_ctrl_ready:
    case CET::telemetry_buffer_full:
    case CET::telemetry_buffer_flushed:
    case CET::prov_started:
    case CET::prov_credentials_received:
    case CET::prov_success:
    case CET::prov_failed:
    case CET::prov_already_done:
    case CET::prov_reset_complete:
    case CET::ota_check_started:
    case CET::ota_download_started:
    case CET::ota_download_progress:
    case CET::ota_download_complete:
    case CET::ota_download_failed:
    case CET::ota_apply_complete:
    case CET::ota_apply_failed:
    case CET::ota_rollback_pending:
    case CET::ota_marked_valid:
        break;

    case CET::wifi_connected:
    case CET::wifi_got_ip:
    case CET::connectivity_ready:
        state.connectivity_ready = true;
        BLUSYS_LOGI(kTag, "connectivity ready");
        break;

    case CET::wifi_disconnected:
    case CET::wifi_reconnecting:
        state.connectivity_ready = false;
        BLUSYS_LOGW(kTag, "connectivity lost");
        break;

    case CET::storage_spiffs_mounted:
    case CET::storage_fatfs_mounted:
    case CET::storage_ready:
        state.storage_ready = true;
        BLUSYS_LOGI(kTag, "storage ready");
        break;

    case CET::diagnostics_ready:
        state.diagnostics_ready = true;
        BLUSYS_LOGI(kTag, "diagnostics ready");
        break;

    case CET::telemetry_ready:
        state.telemetry_ready = true;
        BLUSYS_LOGI(kTag, "telemetry ready");
        break;
    case CET::telemetry_delivered:
        if (const auto *tel = ctx.fx().telemetry.status(); tel != nullptr) {
            state.delivered = tel->total_delivered;
        }
        BLUSYS_LOGI(kTag, "telemetry delivered=%u",
                    static_cast<unsigned>(state.delivered));
        break;
    case CET::telemetry_failed:
        if (const auto *tel = ctx.fx().telemetry.status(); tel != nullptr) {
            state.delivery_fails = tel->total_failed;
        }
        BLUSYS_LOGW(kTag, "telemetry failed=%u",
                    static_cast<unsigned>(state.delivery_fails));
        break;

    case CET::ota_ready:
        state.ota_ready = true;
        BLUSYS_LOGI(kTag, "ota ready");
        break;

    case CET::lan_control_ready:
        state.lan_control_ready = true;
        BLUSYS_LOGI(kTag, "lan control ready");
        break;

    default:
        break;
    }

    refresh_phase(state);
}

}  // namespace surface_gateway
