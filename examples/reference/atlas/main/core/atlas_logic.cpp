#include "core/atlas_logic.hpp"
#include "blusys/blusys.hpp"


#include <optional>

namespace atlas_example {

namespace {

constexpr const char *kTag = "atlas_app";

op_state compute_phase(const app_state &s)
{
    if (s.ota_in_progress)             return op_state::updating;
    if (!s.wifi_connected || !s.has_ip) return op_state::connecting_wifi;
    if (!s.time_synced)                 return op_state::time_syncing;
    if (!s.broker_connected)            return op_state::connecting_broker;
    return op_state::online;
}

void refresh_phase(app_state &s)
{
    op_state next = compute_phase(s);
    if (next != s.phase) {
        BLUSYS_LOGI(kTag, "phase: %s -> %s",
                    op_state_name(s.phase), op_state_name(next));
        s.phase = next;
    }
}

}  // namespace

const char *op_state_name(op_state s)
{
    switch (s) {
    case op_state::idle:              return "idle";
    case op_state::connecting_wifi:   return "connecting_wifi";
    case op_state::time_syncing:      return "time_syncing";
    case op_state::connecting_broker: return "connecting_broker";
    case op_state::online:            return "online";
    case op_state::updating:          return "updating";
    case op_state::error:             return "error";
    }
    return "unknown";
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

    const auto &ce = event.cap_event;

    switch (ce.tag) {
    case CET::wifi_connected:
        state.wifi_connected = true;
        break;
    case CET::wifi_got_ip:
        state.has_ip = true;
        if (const auto *conn = ctx.fx().connectivity.status(); conn != nullptr) {
            BLUSYS_LOGI(kTag, "ip=%s rssi=%d", conn->ip_info.ip, conn->wifi_rssi);
        }
        break;
    case CET::wifi_disconnected:
        state.wifi_connected = false;
        state.has_ip         = false;
        state.time_synced    = false;
        state.broker_connected = false;
        BLUSYS_LOGW(kTag, "wifi dropped — broker will reconnect");
        break;
    case CET::time_synced:
        state.time_synced = true;
        BLUSYS_LOGI(kTag, "SNTP time synced");
        break;
    case CET::time_sync_failed:
        BLUSYS_LOGW(kTag, "SNTP sync failed — continuing with RTC");
        break;
    case CET::ota_download_started:
        state.ota_in_progress  = true;
        state.ota_progress_pct = 0;
        BLUSYS_LOGI(kTag, "OTA download started");
        break;
    case CET::ota_download_progress:
        state.ota_progress_pct = static_cast<std::uint8_t>(ce.value);
        break;
    case CET::ota_download_complete:
        BLUSYS_LOGI(kTag, "OTA download complete");
        break;
    case CET::ota_apply_complete:
        state.ota_in_progress = false;
        BLUSYS_LOGI(kTag, "OTA applied — reboot pending");
        break;
    case CET::ota_download_failed:
    case CET::ota_apply_failed:
        state.ota_in_progress = false;
        BLUSYS_LOGE(kTag, "OTA failed");
        break;

    case CET::integration_passthrough:
        switch (static_cast<atlas_event>(ce.raw_event_id)) {
        case atlas_event::broker_connected:
            state.broker_connected = true;
            BLUSYS_LOGI(kTag, "broker: connected");
            break;
        case atlas_event::broker_disconnected:
            state.broker_connected = false;
            BLUSYS_LOGW(kTag, "broker: disconnected");
            break;
        case atlas_event::command_received:
            state.commands_handled++;
            if (const auto *m = static_cast<const blusys::mqtt_message *>(ce.payload);
                m != nullptr && m->payload_len > 0) {
                BLUSYS_LOGI(kTag, "cmd: %.*s",
                            static_cast<int>(m->payload_len), m->payload);
            }
            break;
        case atlas_event::state_desired:
            if (const auto *m = static_cast<const blusys::mqtt_message *>(ce.payload);
                m != nullptr && m->payload_len > 0) {
                BLUSYS_LOGI(kTag, "state desired: %.*s",
                            static_cast<int>(m->payload_len), m->payload);
            }
            break;
        case atlas_event::ota_announcement:
            if (const auto *m = static_cast<const blusys::mqtt_message *>(ce.payload);
                m != nullptr && m->payload_len > 0) {
                BLUSYS_LOGI(kTag, "ota announce: %.*s",
                            static_cast<int>(m->payload_len), m->payload);
            }
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    refresh_phase(state);
}

}  // namespace atlas_example
