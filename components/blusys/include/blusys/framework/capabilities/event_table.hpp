#pragma once

// Single source of truth for raw integration-event-id → capability_event_tag
// routing. Replaces the per-band switch chains that used to live in
// capability_event_map.cpp.
//
// Adding an event: append a `{raw_id, tag, carry_code}` row and keep the
// table sorted by id. Compile-time `static_assert`s fail the build if the
// table is unsorted, contains duplicate ids, or places an id in a band that
// is not claimed by some `capability_kind` in `kBandOwners`.

#include "blusys/framework/capabilities/capability.hpp"
#include "blusys/framework/capabilities/event.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace blusys {

struct event_mapping {
    std::uint32_t        id;
    capability_event_tag tag;
    // When true, pass the raw event_code into capability_event::value.
    // Currently only used by ota_download_progress (progress percentage).
    bool                 carry_code_as_value;
};

struct band_owner {
    std::uint32_t   band;   // top byte, e.g. 0x0100u
    capability_kind kind;
};

inline constexpr band_owner kBandOwners[] = {
    {0x0100u, capability_kind::connectivity},
    {0x0200u, capability_kind::storage},
    {0x0300u, capability_kind::bluetooth},
    {0x0400u, capability_kind::ota},
    {0x0500u, capability_kind::diagnostics},
    {0x0600u, capability_kind::telemetry},
    {0x0700u, capability_kind::provisioning},
    {0x0800u, capability_kind::mqtt},          // shared with mqtt_host
    {0x0A00u, capability_kind::lan_control},
    {0x0B00u, capability_kind::usb},
    {0x0C00u, capability_kind::ble_hid_device},
};

// Must stay sorted by id. Adding an out-of-order or duplicate entry fails
// the static_asserts below at compile time.
inline constexpr event_mapping kEventTable[] = {
    // connectivity (band 0x0100)
    {0x0100u, capability_event_tag::wifi_started,              false},
    {0x0101u, capability_event_tag::wifi_connecting,           false},
    {0x0102u, capability_event_tag::wifi_connected,            false},
    {0x0103u, capability_event_tag::wifi_got_ip,               false},
    {0x0104u, capability_event_tag::wifi_disconnected,         false},
    {0x0105u, capability_event_tag::wifi_reconnecting,         false},
    {0x0110u, capability_event_tag::time_synced,               false},
    {0x0111u, capability_event_tag::time_sync_failed,          false},
    {0x0120u, capability_event_tag::mdns_ready,                false},
    {0x0130u, capability_event_tag::local_ctrl_ready,          false},
    {0x0140u, capability_event_tag::prov_started,              false},
    {0x0141u, capability_event_tag::prov_credentials_received, false},
    {0x0142u, capability_event_tag::prov_success,              false},
    {0x0143u, capability_event_tag::prov_failed,               false},
    {0x0144u, capability_event_tag::prov_already_done,         false},
    {0x0145u, capability_event_tag::prov_reset_complete,       false},
    {0x014Fu, capability_event_tag::provisioning_ready,        false},
    {0x01FFu, capability_event_tag::connectivity_ready,        false},

    // storage (band 0x0200)
    {0x0200u, capability_event_tag::storage_spiffs_mounted, false},
    {0x0201u, capability_event_tag::storage_fatfs_mounted,  false},
    {0x02FFu, capability_event_tag::storage_ready,          false},

    // bluetooth (band 0x0300)
    {0x0300u, capability_event_tag::bt_gap_ready,              false},
    {0x0301u, capability_event_tag::bt_advertising_started,    false},
    {0x0302u, capability_event_tag::bt_advertising_stopped,    false},
    {0x0303u, capability_event_tag::bt_scan_result,            false},
    {0x0310u, capability_event_tag::bt_gatt_ready,             false},
    {0x0311u, capability_event_tag::bt_client_connected,       false},
    {0x0312u, capability_event_tag::bt_client_disconnected,    false},
    {0x0313u, capability_event_tag::bt_characteristic_written, false},
    {0x03FFu, capability_event_tag::bluetooth_ready,           false},

    // ota (band 0x0400)
    {0x0400u, capability_event_tag::ota_check_started,     false},
    {0x0401u, capability_event_tag::ota_download_started,  false},
    {0x0402u, capability_event_tag::ota_download_progress, true},
    {0x0403u, capability_event_tag::ota_download_complete, false},
    {0x0404u, capability_event_tag::ota_download_failed,   false},
    {0x0405u, capability_event_tag::ota_apply_complete,    false},
    {0x0406u, capability_event_tag::ota_apply_failed,      false},
    {0x0410u, capability_event_tag::ota_rollback_pending,  false},
    {0x0411u, capability_event_tag::ota_marked_valid,      false},
    {0x04FFu, capability_event_tag::ota_ready,             false},

    // diagnostics (band 0x0500)
    {0x0500u, capability_event_tag::diag_snapshot_ready, false},
    {0x0501u, capability_event_tag::diag_console_ready,  false},
    {0x05FFu, capability_event_tag::diagnostics_ready,   false},

    // telemetry (band 0x0600)
    {0x0600u, capability_event_tag::telemetry_buffer_flushed, false},
    {0x0601u, capability_event_tag::telemetry_buffer_full,    false},
    {0x0602u, capability_event_tag::telemetry_delivered,      false},
    {0x0603u, capability_event_tag::telemetry_failed,         false},
    {0x06FFu, capability_event_tag::telemetry_ready,          false},

    // mqtt / mqtt_host (shared band 0x0800)
    {0x0800u, capability_event_tag::mqtt_connected,        false},
    {0x0801u, capability_event_tag::mqtt_disconnected,     false},
    {0x0802u, capability_event_tag::mqtt_message_received, false},
    {0x0803u, capability_event_tag::mqtt_publish_failed,   false},
    {0x080Eu, capability_event_tag::mqtt_error,            false},
    {0x08FFu, capability_event_tag::mqtt_ready,            false},

    // lan_control (band 0x0A00)
    // http_ready and mdns_ready intentionally reuse the tags already
    // emitted by connectivity_capability — products observe one tag for
    // the same semantic regardless of which capability published it.
    {0x0A00u, capability_event_tag::local_ctrl_ready,  false},
    {0x0A01u, capability_event_tag::mdns_ready,        false},
    {0x0AFFu, capability_event_tag::lan_control_ready, false},

    // usb (band 0x0B00) — device_suspended/device_resumed (0x0B12/0x0B13)
    // are intentionally unmapped today; they were explicit `return false`
    // cases in the old switch and stay absent from this table.
    {0x0B00u, capability_event_tag::usb_device_ready,        false},
    {0x0B01u, capability_event_tag::usb_host_ready,          false},
    {0x0B10u, capability_event_tag::usb_device_connected,    false},
    {0x0B11u, capability_event_tag::usb_device_disconnected, false},
    {0x0B20u, capability_event_tag::usb_host_attached,       false},
    {0x0B21u, capability_event_tag::usb_host_detached,       false},
    {0x0BFFu, capability_event_tag::usb_ready,               false},

    // ble_hid_device (band 0x0C00)
    {0x0C01u, capability_event_tag::ble_hid_advertising_started, false},
    {0x0C02u, capability_event_tag::ble_hid_client_connected,    false},
    {0x0C03u, capability_event_tag::ble_hid_client_disconnected, false},
    {0x0CFFu, capability_event_tag::ble_hid_ready,               false},
};

inline constexpr std::size_t kEventTableSize =
    sizeof(kEventTable) / sizeof(kEventTable[0]);
inline constexpr std::size_t kBandOwnerCount =
    sizeof(kBandOwners) / sizeof(kBandOwners[0]);

namespace detail {

constexpr std::uint32_t event_band(std::uint32_t id) noexcept
{
    return id & 0xFF00u;
}

constexpr bool band_is_owned(std::uint32_t band) noexcept
{
    return std::find_if(std::begin(kBandOwners), std::end(kBandOwners),
                        [band](const band_owner &o) { return o.band == band; })
           != std::end(kBandOwners);
}

}  // namespace detail

// std::is_sorted with `<` enforces both sortedness and uniqueness in one
// pass — duplicate ids fail the strict-less comparator.
static_assert(
    std::is_sorted(std::begin(kEventTable), std::end(kEventTable),
                   [](const event_mapping &a, const event_mapping &b) {
                       return a.id < b.id;
                   }),
    "kEventTable must be sorted by id with no duplicates");

static_assert(
    std::all_of(std::begin(kEventTable), std::end(kEventTable),
                [](const event_mapping &m) {
                    return detail::band_is_owned(detail::event_band(m.id));
                }),
    "every kEventTable entry must live in a band listed in kBandOwners");

static_assert(
    [] {
        for (std::size_t i = 0; i < kBandOwnerCount; ++i) {
            for (std::size_t j = i + 1; j < kBandOwnerCount; ++j) {
                if (kBandOwners[i].band == kBandOwners[j].band) return false;
            }
        }
        return true;
    }(),
    "kBandOwners must not declare the same band twice");

}  // namespace blusys
