#pragma once

// Generic SPI ILI9488 320×480 → 480×320 landscape device profile.
//
// Large panels are memory- and bandwidth-sensitive; this profile uses a
// conservative SPI clock and slightly taller draw buffer lines. Validate
// flicker-free operation and DMA sizing on your hardware before shipping.
//
// Override any field after construction.

#include "blusys/framework/platform/profile.hpp"
#include "blusys/drivers/panels/ili9488.h"

namespace blusys::platform {

// Logical 480×320 landscape; native panel is 320×480 with swap_xy enabled.
inline device_profile ili9488_480x320()
{
    device_profile p{};
    p.lcd = blusys_ili9488_default_config();

    p.lcd.spi.bus = 0;

#if CONFIG_IDF_TARGET_ESP32
    p.lcd.spi.sclk_pin = 18;
    p.lcd.spi.mosi_pin = 23;
    p.lcd.spi.cs_pin   = 5;
    p.lcd.spi.dc_pin   = 2;
#elif CONFIG_IDF_TARGET_ESP32C3
    p.lcd.spi.sclk_pin = 4;
    p.lcd.spi.mosi_pin = 6;
    p.lcd.spi.cs_pin   = 7;
    p.lcd.spi.dc_pin   = 3;
#elif CONFIG_IDF_TARGET_ESP32S3
    p.lcd.spi.sclk_pin = 12;
    p.lcd.spi.mosi_pin = 11;
    p.lcd.spi.cs_pin   = 10;
    p.lcd.spi.dc_pin   = 13;
#endif

    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 32;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_DISPLAY_PANEL_KIND_RGB565;

    p.has_encoder = false;
    p.brightness  = 100;

    return p;
}

// Native portrait — 320×480.  swap_xy and mirror_x disabled.
// Note: large portrait panels are very bandwidth-heavy on SPI; validate
// DMA sizing and flicker-free operation before shipping.
inline device_profile ili9488_320x480()
{
    device_profile p = ili9488_480x320();
    p.lcd.width    = BLUSYS_ILI9488_NATIVE_WIDTH;
    p.lcd.height   = BLUSYS_ILI9488_NATIVE_HEIGHT;
    p.lcd.swap_xy  = false;
    p.lcd.mirror_x = false;
    return p;
}

}  // namespace blusys::platform
