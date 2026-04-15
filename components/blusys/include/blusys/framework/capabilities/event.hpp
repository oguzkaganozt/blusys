#pragma once

#include <cstdint>

namespace blusys {

// Canonical capability integration events — one tag space for all subsystems.
// Populated by the framework from raw integration event IDs; reducers typically
// handle these inside `action_tag::capability_event` via `action.cap_event`.
//
// Comments like "connectivity (0x01xx band)" refer to the **reserved raw event ID
// ranges** posted by capabilities (see `capability.hpp`). `capability_event_tag`
// values are **symbolic** — they are not numerically equal to those hex ranges.
enum class capability_event_tag : std::uint16_t {
    none = 0,

    // connectivity (raw IDs: 0x01xx band)
    wifi_started,
    wifi_connecting,
    wifi_connected,
    wifi_got_ip,
    wifi_disconnected,
    wifi_reconnecting,
    time_synced,
    time_sync_failed,
    mdns_ready,
    local_ctrl_ready,
    connectivity_ready,

    // storage (raw IDs: 0x02xx)
    storage_spiffs_mounted,
    storage_fatfs_mounted,
    storage_ready,

    // bluetooth (raw IDs: 0x03xx)
    bt_gap_ready,
    bt_advertising_started,
    bt_advertising_stopped,
    bt_scan_result,
    bt_gatt_ready,
    bt_client_connected,
    bt_client_disconnected,
    bt_characteristic_written,
    bluetooth_ready,

    // ota (raw IDs: 0x04xx)
    ota_check_started,
    ota_download_started,
    ota_download_progress,
    ota_download_complete,
    ota_download_failed,
    ota_apply_complete,
    ota_apply_failed,
    ota_rollback_pending,
    ota_marked_valid,
    ota_ready,

    // diagnostics (raw IDs: 0x05xx)
    diag_snapshot_ready,
    diag_console_ready,
    diagnostics_ready,

    // telemetry (raw IDs: 0x06xx)
    telemetry_delivered,
    telemetry_failed,
    telemetry_buffer_full,
    telemetry_buffer_flushed,
    telemetry_ready,

    // provisioning (raw IDs: 0x07xx)
    prov_started,
    prov_credentials_received,
    prov_success,
    prov_failed,
    prov_already_done,
    prov_reset_complete,
    provisioning_ready,

    // lan_control (raw IDs: 0x0Axx)
    lan_control_ready,

    // usb (raw IDs: 0x0Bxx)
    usb_device_ready,
    usb_host_ready,
    usb_device_connected,
    usb_device_disconnected,
    usb_host_attached,
    usb_host_detached,
    usb_ready,

    // mqtt_host (raw IDs: 0x08xx, host/SDL)
    mqtt_connected,
    mqtt_disconnected,
    mqtt_message_received,
    mqtt_publish_failed,
    mqtt_error,
    mqtt_ready,

    // ble_hid_device (raw IDs: 0x0Cxx)
    ble_hid_advertising_started,
    ble_hid_client_connected,
    ble_hid_client_disconnected,
    ble_hid_ready,

    // `map_integration_event()` had no case; `raw_event_id` / `raw_event_code` hold the
    // original `app_event` id/code when `app_spec::map_event` is non-null.
    integration_passthrough,
};

struct capability_event {
    capability_event_tag tag = capability_event_tag::none;
    std::int32_t         value = 0;  // e.g. OTA download percent
    std::uint32_t        raw_event_id   = 0;
    std::uint32_t        raw_event_code = 0;
    const void          *payload        = nullptr;
};

// Maps a raw integration event (as posted by capabilities) to a canonical event.
// Returns false when `event_id` is not handled here; the runtime may still deliver
// `integration_passthrough` to `app_spec::map_event` (see `app_runtime`).
[[nodiscard]] bool map_integration_event(std::uint32_t event_id, std::uint32_t event_code,
                                         capability_event *out) noexcept;

// Sentinel for app_spec::capability_event_discriminant — auto-wrap disabled.
inline constexpr std::uint32_t k_capability_event_discriminant_unset = 0xFFFFFFFFu;

}  // namespace blusys
