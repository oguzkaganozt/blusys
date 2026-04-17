#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/framework/platform/auto.hpp"
#include "blusys/framework/platform/auto_shell.hpp"
#include "blusys/framework/platform/build.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
namespace handheld_starter::system {

const char *controller_profile_label_for_build()
{
    return blusys::platform::panel_label();
}

const char *controller_hardware_label_for_build()
{
    return blusys::platform::hardware_label();
}

const char *controller_build_version_for_build()
{
    return blusys::platform::version_string();
}

namespace {

static const blusys::shell_config kShellConfig =
    blusys::auto_shell(blusys::auto_profile_interactive(), "Controller");

static blusys::storage_capability storage{{
    .init_nvs        = true,
    .spiffs_base_path = "/controller",
}};

static blusys::connectivity_capability connectivity{{
    .prov_service_name = "pulse-ctrl",
    .prov_pop = "123456",
}};

static blusys::capability_list_storage capabilities{&storage, &connectivity};

static const blusys::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .on_init = ui::on_init,
    .map_intent = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .capabilities = &capabilities,
    .theme = &blusys::presets::expressive_dark(),
    .shell = &kShellConfig,
};

}  // namespace

}  // namespace handheld_starter::system

BLUSYS_APP(handheld_starter::system::spec)
