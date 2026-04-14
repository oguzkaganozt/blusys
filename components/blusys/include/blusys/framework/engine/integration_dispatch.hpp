#pragma once

// Typed helpers for integration events posted to map_event(event_id, event_code, payload).
//
// Wire format (unchanged): event_id is the capability-specific enum value (see ranges in
// capability.hpp). event_code and payload are capability-defined — common uses:
//   - OTA: event_code carries download progress percent for ota_event::download_progress.
//   - Others: typically event_code == 0 and payload == nullptr unless documented on the
//     capability.
//
// Use kind_for_event_id() to branch by subsystem, then as_*_event() to get a typed enum
// without memorizing hex ranges.

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/mqtt_host.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/framework/capabilities/usb.hpp"

#include <cstdint>
#include <optional>

namespace blusys {

// Maps the high byte of event_id to capability_kind.
[[nodiscard]] inline std::optional<capability_kind> kind_for_event_id(std::uint32_t id)
{
    const unsigned hi = (id >> 8) & 0xFFu;
    switch (hi) {
    case 0x01:
        return capability_kind::connectivity;
    case 0x02:
        return capability_kind::storage;
    case 0x03:
        return capability_kind::bluetooth;
    case 0x04:
        return capability_kind::ota;
    case 0x05:
        return capability_kind::diagnostics;
    case 0x06:
        return capability_kind::telemetry;
    case 0x08:
        return capability_kind::mqtt_host;
    case 0x0A:
        return capability_kind::lan_control;
    case 0x0B:
        return capability_kind::usb;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] inline bool as_connectivity_event(std::uint32_t id, connectivity_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<connectivity_event>(id);
    switch (e) {
    case connectivity_event::wifi_started:
    case connectivity_event::wifi_connecting:
    case connectivity_event::wifi_connected:
    case connectivity_event::wifi_got_ip:
    case connectivity_event::wifi_disconnected:
    case connectivity_event::wifi_reconnecting:
    case connectivity_event::time_synced:
    case connectivity_event::time_sync_failed:
    case connectivity_event::mdns_ready:
    case connectivity_event::local_ctrl_ready:
    case connectivity_event::prov_started:
    case connectivity_event::prov_credentials_received:
    case connectivity_event::prov_success:
    case connectivity_event::prov_failed:
    case connectivity_event::prov_already_done:
    case connectivity_event::prov_reset_complete:
    case connectivity_event::provisioning_ready:
    case connectivity_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_storage_event(std::uint32_t id, storage_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<storage_event>(id);
    switch (e) {
    case storage_event::spiffs_mounted:
    case storage_event::fatfs_mounted:
    case storage_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_bluetooth_event(std::uint32_t id, bluetooth_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<bluetooth_event>(id);
    switch (e) {
    case bluetooth_event::gap_ready:
    case bluetooth_event::advertising_started:
    case bluetooth_event::advertising_stopped:
    case bluetooth_event::scan_result:
    case bluetooth_event::gatt_ready:
    case bluetooth_event::client_connected:
    case bluetooth_event::client_disconnected:
    case bluetooth_event::characteristic_written:
    case bluetooth_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_ota_event(std::uint32_t id, ota_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<ota_event>(id);
    switch (e) {
    case ota_event::check_started:
    case ota_event::download_started:
    case ota_event::download_progress:
    case ota_event::download_complete:
    case ota_event::download_failed:
    case ota_event::apply_complete:
    case ota_event::apply_failed:
    case ota_event::rollback_pending:
    case ota_event::marked_valid:
    case ota_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_diagnostics_event(std::uint32_t id, diagnostics_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<diagnostics_event>(id);
    switch (e) {
    case diagnostics_event::snapshot_ready:
    case diagnostics_event::console_ready:
    case diagnostics_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_telemetry_event(std::uint32_t id, telemetry_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<telemetry_event>(id);
    switch (e) {
    case telemetry_event::buffer_flushed:
    case telemetry_event::buffer_full:
    case telemetry_event::delivery_ok:
    case telemetry_event::delivery_failed:
    case telemetry_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_lan_control_event(std::uint32_t id, lan_control_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<lan_control_event>(id);
    switch (e) {
    case lan_control_event::http_ready:
    case lan_control_event::mdns_ready:
    case lan_control_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_usb_event(std::uint32_t id, usb_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<usb_event>(id);
    switch (e) {
    case usb_event::device_ready:
    case usb_event::host_ready:
    case usb_event::device_connected:
    case usb_event::device_disconnected:
    case usb_event::device_suspended:
    case usb_event::device_resumed:
    case usb_event::host_attached:
    case usb_event::host_detached:
    case usb_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline bool as_mqtt_host_event(std::uint32_t id, mqtt_host_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<mqtt_host_event>(id);
    switch (e) {
    case mqtt_host_event::connected:
    case mqtt_host_event::disconnected:
    case mqtt_host_event::message_received:
    case mqtt_host_event::publish_failed:
    case mqtt_host_event::error:
    case mqtt_host_event::capability_ready:
        *out = e;
        return true;
    default:
        return false;
    }
}

// ---- common map_event predicates (optional; keeps integration/*.cpp tables short) ----

[[nodiscard]] inline bool event_is_diagnostics_snapshot_or_ready(std::uint32_t id)
{
    diagnostics_event de;
    if (!as_diagnostics_event(id, &de)) {
        return false;
    }
    return de == diagnostics_event::snapshot_ready ||
           de == diagnostics_event::capability_ready;
}

[[nodiscard]] inline bool event_is_storage_capability_ready(std::uint32_t id)
{
    storage_event se;
    if (!as_storage_event(id, &se)) {
        return false;
    }
    return se == storage_event::capability_ready;
}

}  // namespace blusys
