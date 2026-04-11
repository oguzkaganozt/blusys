// Compile-time and light runtime checks for capability event ranges and
// telemetry readiness sequencing (host stubs).

#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/capability.hpp"

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
    static_assert(u(blusys::app::connectivity_event::wifi_started) >= 0x0100);
    static_assert(u(blusys::app::connectivity_event::capability_ready) <= 0x01FF);
    static_assert(u(blusys::app::storage_event::spiffs_mounted) >= 0x0200);
    static_assert(u(blusys::app::storage_event::capability_ready) <= 0x02FF);
    static_assert(u(blusys::app::bluetooth_event::gap_ready) >= 0x0300);
    static_assert(u(blusys::app::bluetooth_event::capability_ready) <= 0x03FF);
    static_assert(u(blusys::app::ota_event::check_started) >= 0x0400);
    static_assert(u(blusys::app::ota_event::capability_ready) <= 0x04FF);
    static_assert(u(blusys::app::diagnostics_event::snapshot_ready) >= 0x0500);
    static_assert(u(blusys::app::diagnostics_event::capability_ready) <= 0x05FF);
    static_assert(u(blusys::app::telemetry_event::buffer_flushed) >= 0x0600);
    static_assert(u(blusys::app::telemetry_event::capability_ready) <= 0x06FF);
    static_assert(u(blusys::app::provisioning_event::started) >= 0x0700);
    static_assert(u(blusys::app::provisioning_event::capability_ready) <= 0x07FF);

    return EXIT_SUCCESS;
}
