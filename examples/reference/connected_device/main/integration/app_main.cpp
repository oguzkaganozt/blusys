#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/profiles/st7735.hpp"

namespace {

static blusys::app::connectivity_capability conn{{
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
    .sntp_server   = "pool.ntp.org",
    .mdns_hostname = "blusys-device",
}};

static blusys::app::capability_list capabilities{&conn};

static const blusys::app::app_spec<connected_device::State, connected_device::Action> spec{
    .initial_state = {},
    .update        = connected_device::update,
    .on_init       = connected_device::ui::on_init,
    .map_intent     = connected_device::map_intent,
    .map_event      = connected_device::map_event,
    .capabilities   = &capabilities,
};

}  // namespace

BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())
