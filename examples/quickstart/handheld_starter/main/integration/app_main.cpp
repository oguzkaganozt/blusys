#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/auto_profile.hpp"
#include "blusys/app/auto_shell.hpp"
#include "blusys/app/build_profile.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
namespace handheld_starter::system {

const char *controller_profile_label_for_build()
{
    return blusys::app::build_profile::panel_label();
}

const char *controller_hardware_label_for_build()
{
    return blusys::app::build_profile::hardware_label();
}

const char *controller_build_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

static const blusys::app::view::shell_config kShellConfig =
    blusys::app::auto_shell(blusys::app::auto_profile_interactive(), "Controller");

static blusys::app::storage_capability storage{{
    .init_nvs        = true,
    .spiffs_base_path = "/controller",
}};

static blusys::app::connectivity_capability connectivity{{
    .prov_service_name = "pulse-ctrl",
    .prov_pop = "123456",
}};

static blusys::app::capability_list capabilities{&storage, &connectivity};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .on_init = ui::on_init,
    .map_intent = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .capabilities = &capabilities,
    .theme = &blusys::app::presets::expressive_dark(),
    .shell = &kShellConfig,
};

}  // namespace

}  // namespace handheld_starter::system

BLUSYS_APP(handheld_starter::system::spec)
