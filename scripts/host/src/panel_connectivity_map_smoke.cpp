// Mirrors `panel_connectivity_event_triggers_sync` from
// `examples/include/blusys_examples/panel_connectivity_sync.hpp` (included by
// the surface_ops_panel example's map_event bridge).

#include "blusys_examples/panel_connectivity_sync.hpp"

#include "blusys/app/capabilities/diagnostics.hpp"

#include <cstdint>
#include <cstdlib>

namespace {

template<typename E>
constexpr std::uint32_t u(E e)
{
    return static_cast<std::uint32_t>(e);
}

}  // namespace

int main()
{
    using blusys_examples::panel_connectivity_event_triggers_sync;

    if (!panel_connectivity_event_triggers_sync(u(blusys::app::connectivity_event::wifi_got_ip))) {
        return EXIT_FAILURE;
    }
    if (!panel_connectivity_event_triggers_sync(u(blusys::app::connectivity_event::capability_ready))) {
        return EXIT_FAILURE;
    }
    // In-band but not a listed sync event → false (caller falls through to other mappers).
    if (panel_connectivity_event_triggers_sync(0x0106U)) {
        return EXIT_FAILURE;
    }
    // Outside connectivity band.
    if (panel_connectivity_event_triggers_sync(0x0099U)) {
        return EXIT_FAILURE;
    }
    if (panel_connectivity_event_triggers_sync(u(blusys::app::diagnostics_event::snapshot_ready))) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
