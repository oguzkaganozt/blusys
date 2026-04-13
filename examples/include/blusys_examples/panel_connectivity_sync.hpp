#pragma once

#include "blusys/app/capability_event.hpp"
#include "blusys/app/capabilities/connectivity.hpp"

#include <cstdint>

namespace blusys_examples {

/// True when a canonical connectivity tag should trigger a panel connectivity resync
/// (status screen refresh).
inline bool panel_connectivity_event_triggers_sync(blusys::app::capability_event_tag tag)
{
    using CET = blusys::app::capability_event_tag;
    switch (tag) {
    case CET::wifi_started:
    case CET::wifi_connecting:
    case CET::wifi_connected:
    case CET::wifi_got_ip:
    case CET::wifi_disconnected:
    case CET::wifi_reconnecting:
    case CET::time_synced:
    case CET::time_sync_failed:
    case CET::mdns_ready:
    case CET::local_ctrl_ready:
    case CET::connectivity_ready:
        return true;
    default:
        return false;
    }
}

/// True when `id` is a connectivity capability lifecycle ID that should trigger a panel
/// `sync_connectivity` action (see `examples/reference/surface_ops_panel/main/integration/app_main.cpp`).
/// IDs in 0x0100–0x01FF that are not listed return false so `map_event` can fall through.
inline bool panel_connectivity_event_triggers_sync(std::uint32_t id)
{
    if (id < 0x0100U || id > 0x01FFU) {
        return false;
    }
    using CE = blusys::app::connectivity_event;
    switch (static_cast<CE>(id)) {
    case CE::wifi_started:
    case CE::wifi_connecting:
    case CE::wifi_connected:
    case CE::wifi_got_ip:
    case CE::wifi_disconnected:
    case CE::wifi_reconnecting:
    case CE::time_synced:
    case CE::time_sync_failed:
    case CE::mdns_ready:
    case CE::local_ctrl_ready:
    case CE::capability_ready:
        return true;
    default:
        return false;
    }
}

}  // namespace blusys_examples
