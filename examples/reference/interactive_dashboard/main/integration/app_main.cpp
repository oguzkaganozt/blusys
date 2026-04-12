#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/auto_profile.hpp"
#include "blusys/app/auto_shell.hpp"
#include "blusys/app/build_profile.hpp"
#include "blusys/app/theme_presets.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

namespace interactive_dashboard::system {

const char *dashboard_profile_label_for_build()
{
    return blusys::app::dashboard_profile_label();
}

const char *dashboard_hardware_label_for_build()
{
    return blusys::app::dashboard_hardware_label();
}

const char *dashboard_build_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

static const blusys::app::view::shell_config kShellConfig =
    blusys::app::auto_shell_adaptive(blusys::app::auto_profile_dashboard(), "Dashboard");

static blusys::app::diagnostics_capability diagnostics{{
    .enable_console        = false,
    .snapshot_interval_ms  = 1000,
}};

static blusys::app::storage_capability storage{{
    .init_nvs         = true,
    .spiffs_base_path = "/dashbd",
}};

static blusys::app::capability_list capabilities{&diagnostics, &storage};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state    = {},
    .update           = update,
    .on_init          = ui::on_init,
    .on_tick          = on_tick,
    .map_intent       = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms   = 400,
    .capabilities     = &capabilities,
    .theme            = &blusys::app::presets::expressive_dark(),
    .shell            = &kShellConfig,
};

}  // namespace

}  // namespace interactive_dashboard::system

BLUSYS_APP_DASHBOARD(interactive_dashboard::system::spec, "Dashboard")
