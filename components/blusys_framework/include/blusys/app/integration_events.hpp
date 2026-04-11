#pragma once

// Helpers for mapping capability integration event IDs (posted as uint32_t) to
// common patterns — keeps product map_event() small and explicit.
//
// For full discriminated parsing (per-capability enums), see integration_dispatch.hpp
// or include blusys/app/integration.hpp.

#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/capabilities/storage.hpp"

#include <cstdint>

namespace blusys::app::integration {

[[nodiscard]] inline bool is_storage_capability_ready_event(std::uint32_t event_id)
{
    return event_id == static_cast<std::uint32_t>(storage_event::capability_ready);
}

[[nodiscard]] inline bool is_any_provisioning_integration_event(std::uint32_t event_id)
{
    switch (static_cast<provisioning_event>(event_id)) {
    case provisioning_event::started:
    case provisioning_event::credentials_received:
    case provisioning_event::success:
    case provisioning_event::failed:
    case provisioning_event::already_provisioned:
    case provisioning_event::reset_complete:
    case provisioning_event::capability_ready:
        return true;
    default:
        return false;
    }
}

}  // namespace blusys::app::integration
