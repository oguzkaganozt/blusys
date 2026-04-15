#pragma once

// Generic SPI ILI9341 240×320 → 320×240 landscape device profile.
//
// Pre-fills a device_profile for a typical 2.4" ILI9341 SPI breakout in
// landscape. Init and timing live in the HAL; products only adjust pins,
// offsets, or orientation flags to match a specific module.
//
// Override any field after construction.

#include "blusys/framework/platform/profile.hpp"
#include "blusys/drivers/panels/ili9341.h"

namespace blusys::platform {

// Logical 320×240 landscape; native panel is 240×320 with swap_xy enabled.
inline device_profile ili9341_320x240()
{
    device_profile p{};
    p.lcd = blusys_ili9341_default_config();

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
    p.ui.buf_lines    = 24;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_RGB565;

    p.has_encoder = false;
    p.brightness  = 100;

    return p;
}

// Native portrait — 240×320.  swap_xy and mirror_x disabled.
// Typical use: handheld products where the longer edge is vertical.
inline device_profile ili9341_240x320()
{
    device_profile p = ili9341_320x240();
    p.lcd.width    = BLUSYS_ILI9341_NATIVE_WIDTH;
    p.lcd.height   = BLUSYS_ILI9341_NATIVE_HEIGHT;
    p.lcd.swap_xy  = false;
    p.lcd.mirror_x = false;
    return p;
}

}  // namespace blusys::platform
