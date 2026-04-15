#pragma once

// Reference interactive profile selection for device + SDL host — collapses the
// usual ST7735 vs ST7789 vs host matrix into one place per product entry file.
//
// For profile-neutral spellings (version string, shell config, host window), prefer
// `blusys/app/build_profile.hpp` in new integration code.

#include "blusys/framework/platform/layout_surface.hpp"
#include "blusys/framework/platform/profile.hpp"
#include "blusys/hal/version.h"

#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/platform/host.hpp"
#endif

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#include "blusys/framework/platform/profiles/st7735.hpp"
#include "blusys/framework/platform/profiles/st7789.hpp"
#endif

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/composition/shell.hpp"
#endif

namespace blusys::platform {

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_INTERACTIVE_DISPLAY_PROFILE_ST7789) && \
    CONFIG_BLUSYS_INTERACTIVE_DISPLAY_PROFILE_ST7789
inline device_profile interactive_device_profile()
{
    return profiles::st7789_320x240();
}

inline const char *interactive_display_label() { return "ST7789 320x240"; }
inline const char *interactive_hardware_label() { return "ST7789 reference"; }

#elif defined(ESP_PLATFORM)

inline device_profile interactive_device_profile()
{
    return profiles::st7735_160x128();
}

inline const char *interactive_display_label() { return "ST7735 160x128"; }
inline const char *interactive_hardware_label() { return "ST7735 reference"; }

#else

// Host SDL — dimensions follow BLUSYS_IC_HOST_DISPLAY_PROFILE when defined
// (0 = 240×240, 1 = 320×240); see example host/CMakeLists.txt.
inline device_profile interactive_host_logical_profile()
{
    device_profile p{};
#if defined(BLUSYS_IC_HOST_DISPLAY_PROFILE) && (BLUSYS_IC_HOST_DISPLAY_PROFILE == 1)
    p.lcd.width          = 320;
    p.lcd.height         = 240;
#else
    p.lcd.width          = 240;
    p.lcd.height         = 240;
#endif
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_DISPLAY_PANEL_KIND_RGB565;
    return p;
}

inline const char *interactive_display_label()
{
#if defined(BLUSYS_IC_HOST_DISPLAY_PROFILE) && (BLUSYS_IC_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 320x240";
#else
    return "Host SDL 240x240";
#endif
}

inline const char *interactive_hardware_label() { return "Host simulation"; }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
inline ::blusys::host_profile interactive_host_window_profile(const char *window_title = "Pulse Controller")
{
    ::blusys::host_profile hp{};
#if defined(BLUSYS_IC_HOST_DISPLAY_PROFILE) && (BLUSYS_IC_HOST_DISPLAY_PROFILE == 1)
    hp.hor_res = 320;
    hp.ver_res = 240;
#else
    hp.hor_res = 240;
    hp.ver_res = 240;
#endif
    hp.title = window_title;
    return hp;
}
#endif

#endif

inline const char *build_version_string()
{
#ifdef PROJECT_VER
    return PROJECT_VER;
#elif defined(BLUSYS_APP_BUILD_VERSION)
    return BLUSYS_APP_BUILD_VERSION;
#else
    return BLUSYS_VERSION_STRING;
#endif
}

#if defined(BLUSYS_FRAMEWORK_HAS_UI)
inline shell_config interactive_shell_config_for_profile(const device_profile &prof)
{
    const auto            chrome = layout::shell_chrome_for(prof);
    shell_config c{};
    c.header.enabled = true;
    c.header.title   = "Controller";
    c.status.enabled = chrome.status_enabled;
    c.tabs.enabled   = chrome.tabs_enabled;
    return c;
}
#endif

}  // namespace blusys::platform
