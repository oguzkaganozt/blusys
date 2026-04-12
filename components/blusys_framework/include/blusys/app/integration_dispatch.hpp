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

#include "blusys/app/capability.hpp"
#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/telemetry.hpp"

#include <cstdint>
#include <optional>

namespace blusys::app::integration {

// Maps the high byte of event_id to capability_kind (reserved ranges 0x01–0x07).
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
    case 0x07:
        return capability_kind::provisioning;
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

[[nodiscard]] inline bool as_provisioning_event(std::uint32_t id, provisioning_event *out)
{
    if (out == nullptr) {
        return false;
    }
    const auto e = static_cast<provisioning_event>(id);
    switch (e) {
    case provisioning_event::started:
    case provisioning_event::credentials_received:
    case provisioning_event::success:
    case provisioning_event::failed:
    case provisioning_event::already_provisioned:
    case provisioning_event::reset_complete:
    case provisioning_event::capability_ready:
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

}  // namespace blusys::app::integration
