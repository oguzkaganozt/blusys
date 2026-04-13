// Headless reference: connectivity + lan_control (product-shaped, minimal reducer).

#include "core/app_logic.hpp"

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/lan_control.hpp"

namespace headless_lan_control::system {

namespace {

blusys::app::connectivity_config conn_cfg{
#ifdef ESP_PLATFORM
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
#else
    .wifi_ssid     = "host-sim",
    .wifi_password = "",
#endif
    .reconnect_delay_ms = 5000,
    .max_retries        = 10,
    .sntp_server        = "pool.ntp.org",
    .prov_service_name  = "hlan-ctl",
    .prov_pop           = "hlan1234",
    .prov_skip_if_provisioned = true,
};

blusys::app::connectivity_capability connectivity{conn_cfg};

blusys::app::lan_control_capability lan_control{{
    .device_name   = "Headless LAN control",
    .http_port     = 80,
    .service_name  = "blusys-hlan",
    .instance_name = "Blusys headless LAN",
}};

blusys::app::capability_list capabilities{&connectivity, &lan_control};

const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update        = update,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .capabilities  = &capabilities,
};

}  // namespace

}  // namespace headless_lan_control::system

BLUSYS_APP_MAIN_HEADLESS(headless_lan_control::system::spec)
