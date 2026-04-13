#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/auto_profile.hpp"
#include "blusys/app/auto_shell.hpp"
#include "blusys/app/build_profile.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/theme_presets.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

namespace surface_ops_panel::system {

const char *panel_profile_label_for_build()
{
    return blusys::app::dashboard_profile_label();
}

const char *panel_hardware_label_for_build()
{
    return blusys::app::dashboard_hardware_label();
}

const char *panel_build_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

static const blusys::app::view::shell_config kShellConfig =
    blusys::app::auto_shell(blusys::app::auto_profile_dashboard(), "Panel");

blusys::app::connectivity_config panel_connectivity_cfg{
#ifdef ESP_PLATFORM
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
#else
    .wifi_ssid     = "panel-host",
    .wifi_password = "",
#endif
    .reconnect_delay_ms = 3000,
    .max_retries        = 8,
    .sntp_server        = "pool.ntp.org",
    .mdns_hostname      = "blusys-panel",
    .mdns_instance_name = "Blusys Ops Panel",
};

static blusys::app::connectivity_capability connectivity{panel_connectivity_cfg};

static blusys::app::diagnostics_capability diagnostics{{
    .enable_console = false,
    .snapshot_interval_ms = 1000,
}};

static blusys::app::storage_capability storage{{
    // connectivity_capability initializes NVS on device; avoid double init.
    .init_nvs        = false,
    .spiffs_base_path = "/panel",
}};

static blusys::app::capability_list capabilities{&connectivity, &diagnostics, &storage};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .on_init = ui::on_init,
    .on_tick = on_tick,
    .map_intent = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms = 500,
    .capabilities = &capabilities,
    .theme = &blusys::app::presets::operational_light(),
    .shell = &kShellConfig,
};

}  // namespace

}  // namespace surface_ops_panel::system

BLUSYS_APP_DASHBOARD(surface_ops_panel::system::spec, "Ops Panel")
