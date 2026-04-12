#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/build_profile.hpp"
#include "blusys/app/integration.hpp"

namespace interactive_controller::system {

blusys::app::device_profile controller_device_profile_for_build()
{
#if defined(ESP_PLATFORM)
    return blusys::app::build_profile::device_profile_for_interactive();
#else
    return blusys::app::build_profile::host_logical_profile();
#endif
}

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
    blusys::app::build_profile::shell_config_for_device_profile(
        controller_device_profile_for_build());

bool map_event(std::uint32_t id, std::uint32_t /*code*/, const void * /*payload*/, action *out)
{
    using namespace blusys::app::integration;
    using blusys::app::provisioning_event;
    using blusys::app::storage_event;
    storage_event storage_e;
    if (as_storage_event(id, &storage_e) && storage_e == storage_event::capability_ready) {
        *out = action{.tag = action_tag::sync_storage};
        return true;
    }
    provisioning_event prov_e;
    if (as_provisioning_event(id, &prov_e)) {
        *out = action{.tag = action_tag::sync_provisioning};
        return true;
    }
    return false;
}

static blusys::app::storage_capability storage{{
    .init_nvs        = true,
    .spiffs_base_path = "/controller",
}};

static blusys::app::provisioning_capability provisioning{{
    .service_name = "pulse-ctrl",
    .pop = "123456",
}};

static blusys::app::capability_list capabilities{&storage, &provisioning};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .on_init = ui::on_init,
    .map_intent = map_intent,
    .map_event = map_event,
    .capabilities = &capabilities,
    .theme = &blusys::app::presets::expressive_dark(),
    .shell = &kShellConfig,
};

}  // namespace

}  // namespace interactive_controller::system

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(interactive_controller::system::spec,
                       interactive_controller::system::controller_device_profile_for_build())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(
    interactive_controller::system::spec,
    blusys::app::build_profile::host_window("Pulse Controller"))
#endif
