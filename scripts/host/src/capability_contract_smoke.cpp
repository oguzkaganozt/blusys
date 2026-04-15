// Compile-time and light runtime checks for capability event ranges and
// telemetry readiness sequencing (host stubs).

#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/framework/capabilities/usb.hpp"
#include "blusys/framework/capabilities/capability.hpp"

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
    static_assert(u(blusys::connectivity_event::wifi_started) >= 0x0100);
    static_assert(u(blusys::connectivity_event::capability_ready) <= 0x01FF);
    static_assert(u(blusys::storage_event::spiffs_mounted) >= 0x0200);
    static_assert(u(blusys::storage_event::capability_ready) <= 0x02FF);
    static_assert(u(blusys::bluetooth_event::gap_ready) >= 0x0300);
    static_assert(u(blusys::bluetooth_event::capability_ready) <= 0x03FF);
    static_assert(u(blusys::ota_event::check_started) >= 0x0400);
    static_assert(u(blusys::ota_event::capability_ready) <= 0x04FF);
    static_assert(u(blusys::diagnostics_event::snapshot_ready) >= 0x0500);
    static_assert(u(blusys::diagnostics_event::capability_ready) <= 0x05FF);
    static_assert(u(blusys::telemetry_event::buffer_flushed) >= 0x0600);
    static_assert(u(blusys::telemetry_event::capability_ready) <= 0x06FF);
    static_assert(u(blusys::lan_control_event::http_ready) >= 0x0A00);
    static_assert(u(blusys::lan_control_event::capability_ready) <= 0x0AFF);
    static_assert(u(blusys::usb_event::device_ready) >= 0x0B00);
    static_assert(u(blusys::usb_event::capability_ready) <= 0x0BFF);

    return EXIT_SUCCESS;
}
