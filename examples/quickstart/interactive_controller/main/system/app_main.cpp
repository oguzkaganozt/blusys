#include "logic/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/theme_presets.hpp"

#include <cstdint>

namespace interactive_controller::system {

namespace {

constexpr blusys::app::view::shell_config kShellConfig{
    .header = {.enabled = true, .title = "Controller"},
    .status = {.enabled = true},
    .tabs = {.enabled = true},
};

bool map_event(std::uint32_t id, std::uint32_t /*code*/, const void * /*payload*/, action *out)
{
    switch (id) {
    case static_cast<std::uint32_t>(blusys::app::storage_event::bundle_ready):
        *out = action{.tag = action_tag::sync_storage};
        return true;
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::started):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::credentials_received):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::success):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::failed):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::already_provisioned):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::reset_complete):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::bundle_ready):
        *out = action{.tag = action_tag::sync_provisioning};
        return true;
    default:
        return false;
    }
}

static blusys::app::storage_capability storage{{
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
#include "blusys/app/profiles/st7735.hpp"
BLUSYS_APP_MAIN_DEVICE(interactive_controller::system::spec,
                       blusys::app::profiles::st7735_160x128())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(interactive_controller::system::spec,
                             (blusys::app::host_profile{
                                 .hor_res = 240,
                                 .ver_res = 240,
                                 .title = "Pulse Controller",
                             }))
#endif
