#pragma once

// Central profile selection for interactive and dashboard-class examples.
// Device: uses sdkconfig from components/blusys_framework/Kconfig.
// Host SDL: logical size follows BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE or
// BLUSYS_IC_HOST_DISPLAY_PROFILE (interactive) from host/CMakeLists.txt.

#include "blusys/app/host_profile.hpp"
#include "blusys/app/platform_profile.hpp"
#include "blusys/app/reference_build_profile.hpp"

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#include "blusys/app/profiles/ili9341.hpp"
#include "blusys/app/profiles/ili9488.hpp"
#include "blusys/app/profiles/qemu_rgb.hpp"
#endif

namespace blusys::app {

// ---- interactive controller (ST7735 / ST7789 + host) ----

#if defined(ESP_PLATFORM)

[[nodiscard]] inline device_profile auto_profile_interactive()
{
    return reference_build::interactive_device_profile();
}

#else

[[nodiscard]] inline device_profile auto_profile_interactive()
{
    return reference_build::interactive_host_logical_profile();
}

#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && !defined(ESP_PLATFORM)

[[nodiscard]] inline host_profile auto_host_profile_interactive(const char *window_title = nullptr)
{
    return reference_build::interactive_host_window_profile(
        window_title != nullptr ? window_title : "Pulse Controller");
}

#endif

// ---- dashboard (device: SPI ILI9341 / ILI9488 or QEMU RGB; host: SDL) ----

[[nodiscard]] inline device_profile auto_profile_dashboard()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB) && \
    CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB
    return profiles::qemu_rgb_dashboard_320x240();
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488
    return profiles::ili9488_480x320();
#elif defined(ESP_PLATFORM)
    return profiles::ili9341_320x240();
#else
    device_profile p{};
#if defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 1)
    p.lcd.width  = 480;
    p.lcd.height = 320;
#else
    p.lcd.width  = 320;
    p.lcd.height = 240;
#endif
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_UI_PANEL_KIND_RGB565;
    return p;
#endif
}

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && !defined(ESP_PLATFORM)

[[nodiscard]] inline host_profile auto_host_profile_dashboard(const char *title)
{
    host_profile hp{};
#if defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 1)
    hp.hor_res = 480;
    hp.ver_res = 320;
#else
    hp.hor_res = 320;
    hp.ver_res = 240;
#endif
    hp.title = title;
    return hp;
}

#endif

[[nodiscard]] inline const char *dashboard_profile_label()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB) && \
    CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB
    return "QEMU RGB 320x240";
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488
    return "ILI9488 480x320";
#elif defined(ESP_PLATFORM)
    return "ILI9341 320x240";
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 480x320";
#else
    return "Host SDL 320x240";
#endif
}

[[nodiscard]] inline const char *dashboard_hardware_label()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB) && \
    CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB
    return "QEMU virtual panel";
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488
    return "ILI9488 reference";
#elif defined(ESP_PLATFORM)
    return "ILI9341 reference";
#else
    return "Host simulation";
#endif
}

}  // namespace blusys::app
