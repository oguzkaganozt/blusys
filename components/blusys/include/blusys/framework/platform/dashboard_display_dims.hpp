#pragma once

// Single-source dashboard display dimensions, dispatched from Kconfig (ESP)
// or BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE (host SDL).
//
// Consumers:
//   - auto.hpp (host-path fallback in auto_profile_dashboard /
//               auto_host_profile_dashboard)
//   - display_constants.hpp (kDisplayWidth / kDisplayHeight for if constexpr
//               branching)
//
// The ESP-path profile functions (profiles::ili9341_320x240 etc.) do not
// consume these macros — their dimensions live in their function names.
// Reviewers must keep the two in sync when adding a new dashboard profile;
// a single new profile requires editing exactly this header plus the matching
// profiles::*_WxH().

#ifndef BLUSYS_DEVICE_BUILD
#define BLUSYS_DEVICE_BUILD 0
#endif

#if BLUSYS_DEVICE_BUILD
#include "sdkconfig.h"
#endif

#if BLUSYS_DEVICE_BUILD && defined(CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB) && \
    CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB
#define BLUSYS_DASHBOARD_DISPLAY_W 320
#define BLUSYS_DASHBOARD_DISPLAY_H 240
#elif BLUSYS_DEVICE_BUILD && \
    defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488
#define BLUSYS_DASHBOARD_DISPLAY_W 480
#define BLUSYS_DASHBOARD_DISPLAY_H 320
#elif BLUSYS_DEVICE_BUILD && \
    defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735
#define BLUSYS_DASHBOARD_DISPLAY_W 160
#define BLUSYS_DASHBOARD_DISPLAY_H 128
#elif BLUSYS_DEVICE_BUILD && \
    defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
#define BLUSYS_DASHBOARD_DISPLAY_W 128
#define BLUSYS_DASHBOARD_DISPLAY_H 64
#elif BLUSYS_DEVICE_BUILD
// Default ESP profile: ILI9341 320x240.
#define BLUSYS_DASHBOARD_DISPLAY_W 320
#define BLUSYS_DASHBOARD_DISPLAY_H 240
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && \
    (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 1)
#define BLUSYS_DASHBOARD_DISPLAY_W 480
#define BLUSYS_DASHBOARD_DISPLAY_H 320
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && \
    (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 2)
#define BLUSYS_DASHBOARD_DISPLAY_W 160
#define BLUSYS_DASHBOARD_DISPLAY_H 128
#else
// Default host: 320x240.
#define BLUSYS_DASHBOARD_DISPLAY_W 320
#define BLUSYS_DASHBOARD_DISPLAY_H 240
#endif
