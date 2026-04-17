#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"
#include "blusys/blusys.hpp"


namespace {

static blusys::connectivity_capability conn{{
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
    .sntp_server   = "pool.ntp.org",
    .mdns_hostname = "blusys-device",
}};

static blusys::capability_list_storage capabilities{&conn};

static const blusys::device_profile kProfile = blusys::platform::st7735_160x128();

static const blusys::app_spec<connected_device::State, connected_device::Action> spec{
    .initial_state = {},
    .update        = connected_device::update,
    .on_init       = connected_device::ui::on_init,
    .on_event      = connected_device::on_event,
    .capabilities  = &capabilities,
    .profile       = &kProfile,
};

}  // namespace

BLUSYS_APP(spec)
