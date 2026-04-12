#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/build_profile.hpp"
#include "blusys/app/layout_surface.hpp"
#include "blusys/app/theme_presets.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/app/profiles/ili9341.hpp"
#include "blusys/app/profiles/ili9488.hpp"
#endif

namespace interactive_dashboard::system {

blusys::app::device_profile dashboard_device_profile_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488
    return blusys::app::profiles::ili9488_480x320();
#elif defined(ESP_PLATFORM)
    return blusys::app::profiles::ili9341_320x240();
#else
    blusys::app::device_profile p{};
#if defined(BLUSYS_IDASH_HOST_DISPLAY_PROFILE) && (BLUSYS_IDASH_HOST_DISPLAY_PROFILE == 1)
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

const char *dashboard_profile_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488
    return "ILI9488 480x320";
#elif defined(ESP_PLATFORM)
    return "ILI9341 320x240";
#elif defined(BLUSYS_IDASH_HOST_DISPLAY_PROFILE) && (BLUSYS_IDASH_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 480x320";
#else
    return "Host SDL 320x240";
#endif
}

const char *dashboard_hardware_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488
    return "ILI9488 reference";
#elif defined(ESP_PLATFORM)
    return "ILI9341 reference";
#else
    return "Host simulation";
#endif
}

const char *dashboard_build_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

const blusys::app::view::shell_config dashboard_shell_for_profile()
{
    const auto prof = dashboard_device_profile_for_build();
    const auto chrome = blusys::app::layout::shell_chrome_for(prof);
    blusys::app::view::shell_config c{};
    c.header.enabled = true;
    c.header.title   = "Dashboard";
    c.status.enabled = chrome.status_enabled;
    c.tabs.enabled   = chrome.tabs_enabled;

    // ILI9341-class (and narrow host) logical heights leave little room for content;
    // shrink chrome so the scroll viewport matches user expectations.
    const bool compact_chrome =
        (prof.lcd.height <= 280) || (prof.lcd.width <= 360 && prof.lcd.height <= 300);
    if (compact_chrome) {
        c.header.height = 30;
        c.status.height = 16;
        c.tabs.height   = 28;
    }
    return c;
}

static const blusys::app::view::shell_config kShellConfig = dashboard_shell_for_profile();

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

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(interactive_dashboard::system::spec,
                       interactive_dashboard::system::dashboard_device_profile_for_build())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(
    interactive_dashboard::system::spec,
    (blusys::app::host_profile{
#if defined(BLUSYS_IDASH_HOST_DISPLAY_PROFILE) && (BLUSYS_IDASH_HOST_DISPLAY_PROFILE == 1)
        .hor_res = 480,
        .ver_res = 320,
#else
        .hor_res = 320,
        .ver_res = 240,
#endif
        .title = "Dashboard",
    }))
#endif
