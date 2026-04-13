#include "blusys/app/capability_event.hpp"

#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/lan_control.hpp"
#include "blusys/app/capabilities/mqtt_host.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/capabilities/usb.hpp"

namespace blusys::app {
namespace {

constexpr std::uint32_t kBandMask = 0xFF00u;

}  // namespace

bool map_integration_event(std::uint32_t event_id, std::uint32_t event_code,
                           capability_event *out) noexcept
{
    if (out == nullptr) {
        return false;
    }

    const std::uint32_t band = event_id & kBandMask;

    if (band == 0x0100u) {
        switch (static_cast<connectivity_event>(event_id)) {
        case connectivity_event::wifi_started:
            *out = {capability_event_tag::wifi_started, 0};
            return true;
        case connectivity_event::wifi_connecting:
            *out = {capability_event_tag::wifi_connecting, 0};
            return true;
        case connectivity_event::wifi_connected:
            *out = {capability_event_tag::wifi_connected, 0};
            return true;
        case connectivity_event::wifi_got_ip:
            *out = {capability_event_tag::wifi_got_ip, 0};
            return true;
        case connectivity_event::wifi_disconnected:
            *out = {capability_event_tag::wifi_disconnected, 0};
            return true;
        case connectivity_event::wifi_reconnecting:
            *out = {capability_event_tag::wifi_reconnecting, 0};
            return true;
        case connectivity_event::time_synced:
            *out = {capability_event_tag::time_synced, 0};
            return true;
        case connectivity_event::time_sync_failed:
            *out = {capability_event_tag::time_sync_failed, 0};
            return true;
        case connectivity_event::mdns_ready:
            *out = {capability_event_tag::mdns_ready, 0};
            return true;
        case connectivity_event::local_ctrl_ready:
            *out = {capability_event_tag::local_ctrl_ready, 0};
            return true;
        case connectivity_event::prov_started:
            *out = {capability_event_tag::prov_started, 0};
            return true;
        case connectivity_event::prov_credentials_received:
            *out = {capability_event_tag::prov_credentials_received, 0};
            return true;
        case connectivity_event::prov_success:
            *out = {capability_event_tag::prov_success, 0};
            return true;
        case connectivity_event::prov_failed:
            *out = {capability_event_tag::prov_failed, 0};
            return true;
        case connectivity_event::prov_already_done:
            *out = {capability_event_tag::prov_already_done, 0};
            return true;
        case connectivity_event::prov_reset_complete:
            *out = {capability_event_tag::prov_reset_complete, 0};
            return true;
        case connectivity_event::provisioning_ready:
            *out = {capability_event_tag::provisioning_ready, 0};
            return true;
        case connectivity_event::capability_ready:
            *out = {capability_event_tag::connectivity_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0200u) {
        switch (static_cast<storage_event>(event_id)) {
        case storage_event::spiffs_mounted:
            *out = {capability_event_tag::storage_spiffs_mounted, 0};
            return true;
        case storage_event::fatfs_mounted:
            *out = {capability_event_tag::storage_fatfs_mounted, 0};
            return true;
        case storage_event::capability_ready:
            *out = {capability_event_tag::storage_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0300u) {
        switch (static_cast<bluetooth_event>(event_id)) {
        case bluetooth_event::gap_ready:
            *out = {capability_event_tag::bt_gap_ready, 0};
            return true;
        case bluetooth_event::advertising_started:
            *out = {capability_event_tag::bt_advertising_started, 0};
            return true;
        case bluetooth_event::advertising_stopped:
            *out = {capability_event_tag::bt_advertising_stopped, 0};
            return true;
        case bluetooth_event::scan_result:
            *out = {capability_event_tag::bt_scan_result, 0};
            return true;
        case bluetooth_event::gatt_ready:
            *out = {capability_event_tag::bt_gatt_ready, 0};
            return true;
        case bluetooth_event::client_connected:
            *out = {capability_event_tag::bt_client_connected, 0};
            return true;
        case bluetooth_event::client_disconnected:
            *out = {capability_event_tag::bt_client_disconnected, 0};
            return true;
        case bluetooth_event::characteristic_written:
            *out = {capability_event_tag::bt_characteristic_written, 0};
            return true;
        case bluetooth_event::capability_ready:
            *out = {capability_event_tag::bluetooth_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0400u) {
        switch (static_cast<ota_event>(event_id)) {
        case ota_event::check_started:
            *out = {capability_event_tag::ota_check_started, 0};
            return true;
        case ota_event::download_started:
            *out = {capability_event_tag::ota_download_started, 0};
            return true;
        case ota_event::download_progress:
            *out = {capability_event_tag::ota_download_progress,
                    static_cast<std::int32_t>(event_code)};
            return true;
        case ota_event::download_complete:
            *out = {capability_event_tag::ota_download_complete, 0};
            return true;
        case ota_event::download_failed:
            *out = {capability_event_tag::ota_download_failed, 0};
            return true;
        case ota_event::apply_complete:
            *out = {capability_event_tag::ota_apply_complete, 0};
            return true;
        case ota_event::apply_failed:
            *out = {capability_event_tag::ota_apply_failed, 0};
            return true;
        case ota_event::rollback_pending:
            *out = {capability_event_tag::ota_rollback_pending, 0};
            return true;
        case ota_event::marked_valid:
            *out = {capability_event_tag::ota_marked_valid, 0};
            return true;
        case ota_event::capability_ready:
            *out = {capability_event_tag::ota_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0500u) {
        switch (static_cast<diagnostics_event>(event_id)) {
        case diagnostics_event::snapshot_ready:
            *out = {capability_event_tag::diag_snapshot_ready, 0};
            return true;
        case diagnostics_event::console_ready:
            *out = {capability_event_tag::diag_console_ready, 0};
            return true;
        case diagnostics_event::capability_ready:
            *out = {capability_event_tag::diagnostics_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0600u) {
        switch (static_cast<telemetry_event>(event_id)) {
        case telemetry_event::buffer_flushed:
            *out = {capability_event_tag::telemetry_buffer_flushed, 0};
            return true;
        case telemetry_event::buffer_full:
            *out = {capability_event_tag::telemetry_buffer_full, 0};
            return true;
        case telemetry_event::delivery_ok:
            *out = {capability_event_tag::telemetry_delivered, 0};
            return true;
        case telemetry_event::delivery_failed:
            *out = {capability_event_tag::telemetry_failed, 0};
            return true;
        case telemetry_event::capability_ready:
            *out = {capability_event_tag::telemetry_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0800u) {
        switch (static_cast<mqtt_host_event>(event_id)) {
        case mqtt_host_event::connected:
            *out = {capability_event_tag::mqtt_connected, 0};
            return true;
        case mqtt_host_event::disconnected:
            *out = {capability_event_tag::mqtt_disconnected, 0};
            return true;
        case mqtt_host_event::message_received:
            *out = {capability_event_tag::mqtt_message_received, 0};
            return true;
        case mqtt_host_event::publish_failed:
            *out = {capability_event_tag::mqtt_publish_failed, 0};
            return true;
        case mqtt_host_event::error:
            *out = {capability_event_tag::mqtt_error, 0};
            return true;
        case mqtt_host_event::capability_ready:
            *out = {capability_event_tag::mqtt_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0A00u) {
        switch (static_cast<lan_control_event>(event_id)) {
        case lan_control_event::http_ready:
            *out = {capability_event_tag::local_ctrl_ready, 0};
            return true;
        case lan_control_event::mdns_ready:
            *out = {capability_event_tag::mdns_ready, 0};
            return true;
        case lan_control_event::capability_ready:
            *out = {capability_event_tag::lan_control_ready, 0};
            return true;
        default:
            break;
        }
    }

    if (band == 0x0B00u) {
        switch (static_cast<usb_event>(event_id)) {
        case usb_event::device_ready:
            *out = {capability_event_tag::usb_device_ready, 0};
            return true;
        case usb_event::host_ready:
            *out = {capability_event_tag::usb_host_ready, 0};
            return true;
        case usb_event::device_connected:
            *out = {capability_event_tag::usb_device_connected, 0};
            return true;
        case usb_event::device_disconnected:
            *out = {capability_event_tag::usb_device_disconnected, 0};
            return true;
        case usb_event::host_attached:
            *out = {capability_event_tag::usb_host_attached, 0};
            return true;
        case usb_event::host_detached:
            *out = {capability_event_tag::usb_host_detached, 0};
            return true;
        case usb_event::device_suspended:
        case usb_event::device_resumed:
            return false;
        case usb_event::capability_ready:
            *out = {capability_event_tag::usb_ready, 0};
            return true;
        default:
            break;
        }
    }

    return false;
}

}  // namespace blusys::app
