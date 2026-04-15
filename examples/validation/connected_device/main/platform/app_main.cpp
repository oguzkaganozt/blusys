#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/platform/profiles/st7735.hpp"

namespace {

static blusys::connectivity_capability conn{{
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
    .sntp_server   = "pool.ntp.org",
    .mdns_hostname = "blusys-device",
}};

static blusys::capability_list capabilities{&conn};

static const blusys::app_spec<connected_device::State, connected_device::Action> spec{
    .initial_state = {},
    .update        = connected_device::update,
    .on_init       = connected_device::ui::on_init,
    .map_intent     = connected_device::map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(connected_device::Tag::capability_event),
    .capabilities   = &capabilities,
};

}  // namespace

BLUSYS_APP_MAIN_DEVICE(spec, blusys::platform::st7735_160x128())
