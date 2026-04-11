#pragma once

#include "blusys/app/capabilities/connectivity.hpp"

#include <cstdint>

namespace blusys_examples {

/// True when `id` is a connectivity capability lifecycle ID that should trigger a panel
/// `sync_connectivity` action (see `examples/reference/interactive_panel/main/integration/app_main.cpp`).
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
