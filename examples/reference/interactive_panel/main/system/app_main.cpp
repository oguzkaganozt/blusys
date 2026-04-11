#include "logic/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/layout_surface.hpp"
#include "blusys/app/theme_presets.hpp"
#include "blusys/version.h"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/app/profiles/ili9341.hpp"
#include "blusys/app/profiles/ili9488.hpp"
#endif

namespace interactive_panel::system {

blusys::app::device_profile panel_device_profile_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IP_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IP_DISPLAY_PROFILE_ILI9488
    return blusys::app::profiles::ili9488_480x320();
#elif defined(ESP_PLATFORM)
    return blusys::app::profiles::ili9341_320x240();
#else
    blusys::app::device_profile p{};
#if defined(BLUSYS_IP_HOST_DISPLAY_PROFILE) && (BLUSYS_IP_HOST_DISPLAY_PROFILE == 1)
    p.lcd.width          = 480;
    p.lcd.height         = 320;
#else
    p.lcd.width          = 320;
    p.lcd.height         = 240;
#endif
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_UI_PANEL_KIND_RGB565;
    return p;
#endif
}

const char *panel_profile_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IP_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IP_DISPLAY_PROFILE_ILI9488
    return "ILI9488 480x320";
#elif defined(ESP_PLATFORM)
    return "ILI9341 320x240";
#elif defined(BLUSYS_IP_HOST_DISPLAY_PROFILE) && (BLUSYS_IP_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 480x320";
#else
    return "Host SDL 320x240";
#endif
}

const char *panel_hardware_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IP_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IP_DISPLAY_PROFILE_ILI9488
    return "ILI9488 reference";
#elif defined(ESP_PLATFORM)
    return "ILI9341 reference";
#else
    return "Host simulation";
#endif
}

const char *panel_build_version_for_build()
{
#ifdef PROJECT_VER
    return PROJECT_VER;
#elif defined(BLUSYS_APP_BUILD_VERSION)
    return BLUSYS_APP_BUILD_VERSION;
#else
    return BLUSYS_VERSION_STRING;
#endif
}

namespace {

const blusys::app::view::shell_config panel_shell_for_profile()
{
    const auto prof = panel_device_profile_for_build();
    const auto chrome = blusys::app::layout::shell_chrome_for(prof);
    blusys::app::view::shell_config c{};
    c.header.enabled = true;
    c.header.title   = "Panel";
    c.status.enabled = chrome.status_enabled;
    c.tabs.enabled   = chrome.tabs_enabled;
    return c;
}

static const blusys::app::view::shell_config kShellConfig = panel_shell_for_profile();

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
BLUSYS_APP_MAIN_DEVICE(interactive_panel::system::spec,
                       interactive_panel::system::panel_device_profile_for_build())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(
    interactive_panel::system::spec,
    (blusys::app::host_profile{
#if defined(BLUSYS_IP_HOST_DISPLAY_PROFILE) && (BLUSYS_IP_HOST_DISPLAY_PROFILE == 1)
        .hor_res = 480,
        .ver_res = 320,
#else
        .hor_res = 320,
        .ver_res = 240,
#endif
        .title = "Ops Panel",
    }))
#endif
