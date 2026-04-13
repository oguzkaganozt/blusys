// Minimal headless validation: connectivity + lan_control only.

#include "core/app_logic.hpp"

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/lan_control.hpp"

namespace lan_control_basic::system {

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
    .prov_service_name  = "lan-ctl-basic",
    .prov_pop           = "lan12345",
    .prov_skip_if_provisioned = true,
};

blusys::app::connectivity_capability connectivity{conn_cfg};

blusys::app::lan_control_capability lan_control{{
    .device_name   = "LAN Control Basic",
    .http_port     = 80,
    .service_name  = "blusys-lc-basic",
    .instance_name = "Blusys LAN Control Basic",
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

}  // namespace lan_control_basic::system

BLUSYS_APP_MAIN_HEADLESS(lan_control_basic::system::spec)
