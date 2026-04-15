#pragma once

// Central profile selection for interactive and dashboard-class examples.
// Device: uses sdkconfig from components/blusys_framework/Kconfig.
// Host SDL: logical size follows BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE or
// BLUSYS_IC_HOST_DISPLAY_PROFILE (interactive) from host/CMakeLists.txt.

#include "blusys/framework/platform/host.hpp"
#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/platform/reference_build.hpp"
#include "blusys/framework/platform/dashboard_display_dims.hpp"

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#include "blusys/framework/platform/profiles/ili9341.hpp"
#include "blusys/framework/platform/profiles/ili9488.hpp"
#include "blusys/framework/platform/profiles/qemu_rgb.hpp"
#include "blusys/framework/platform/profiles/st7735.hpp"
#include "blusys/framework/platform/profiles/ssd1306.hpp"
#endif

namespace blusys {

// ---- handheld interactive (ST7735 / ST7789 + host) ----

#if defined(ESP_PLATFORM)

[[nodiscard]] inline device_profile auto_profile_interactive()
{
    return blusys::platform::interactive_device_profile();
}

#else

[[nodiscard]] inline device_profile auto_profile_interactive()
{
    return blusys::platform::interactive_host_logical_profile();
}

#endif

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && !defined(ESP_PLATFORM)

[[nodiscard]] inline host_profile auto_host_profile_interactive(const char *window_title = nullptr)
{
    return blusys::platform::interactive_host_window_profile(
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
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735
    return profiles::st7735_160x128();
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
    return profiles::ssd1306_128x64();
#elif defined(ESP_PLATFORM)
    return profiles::ili9341_320x240();
#else
    device_profile p{};
    p.lcd.width          = BLUSYS_DASHBOARD_DISPLAY_W;
    p.lcd.height         = BLUSYS_DASHBOARD_DISPLAY_H;
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_DISPLAY_PANEL_KIND_RGB565;
    return p;
#endif
}

#if defined(BLUSYS_FRAMEWORK_HAS_UI) && !defined(ESP_PLATFORM)

[[nodiscard]] inline host_profile auto_host_profile_dashboard(const char *title)
{
    host_profile hp{};
    hp.hor_res = BLUSYS_DASHBOARD_DISPLAY_W;
    hp.ver_res = BLUSYS_DASHBOARD_DISPLAY_H;
    hp.title   = title;
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
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735
    return "ST7735 160x128";
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
    return "SSD1306 128x64";
#elif defined(ESP_PLATFORM)
    return "ILI9341 320x240";
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 480x320";
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 2)
    return "Host SDL 160x128";
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
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735
    return "ST7735 reference";
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
    return "SSD1306 I2C OLED";
#elif defined(ESP_PLATFORM)
    return "ILI9341 reference";
#else
    return "Host simulation";
#endif
}

}  // namespace blusys
