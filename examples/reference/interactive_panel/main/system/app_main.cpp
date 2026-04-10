#include "logic/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/theme_presets.hpp"

#include <cstdint>

namespace interactive_panel::system {

namespace {

constexpr blusys::app::view::shell_config kShellConfig{
    .header = {.enabled = true, .title = "Panel"},
    .status = {.enabled = true},
    .tabs = {.enabled = true},
};

bool map_event(std::uint32_t id, std::uint32_t /*code*/, const void * /*payload*/, action *out)
{
    switch (id) {
    case static_cast<std::uint32_t>(blusys::app::diagnostics_event::snapshot_ready):
    case static_cast<std::uint32_t>(blusys::app::diagnostics_event::bundle_ready):
        *out = action{.tag = action_tag::sync_diagnostics};
        return true;
    case static_cast<std::uint32_t>(blusys::app::storage_event::bundle_ready):
        *out = action{.tag = action_tag::sync_storage};
        return true;
    default:
        return false;
    }
}

static blusys::app::diagnostics_capability diagnostics{{
    .enable_console = false,
    .snapshot_interval_ms = 1000,
}};

static blusys::app::storage_capability storage{{
    .spiffs_base_path = "/panel",
}};

static blusys::app::capability_list capabilities{&diagnostics, &storage};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .on_init = ui::on_init,
    .on_tick = on_tick,
    .map_intent = map_intent,
    .map_event = map_event,
    .tick_period_ms = 500,
    .capabilities = &capabilities,
    .theme = &blusys::app::presets::operational_light(),
    .shell = &kShellConfig,
};

}  // namespace

}  // namespace interactive_panel::system

#ifdef ESP_PLATFORM
#include "blusys/app/profiles/st7735.hpp"
BLUSYS_APP_MAIN_DEVICE(interactive_panel::system::spec,
                       blusys::app::profiles::st7735_160x128())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(interactive_panel::system::spec,
                             (blusys::app::host_profile{
                                 .hor_res = 320,
                                 .ver_res = 240,
                                 .title = "Ops Panel",
                             }))
#endif
