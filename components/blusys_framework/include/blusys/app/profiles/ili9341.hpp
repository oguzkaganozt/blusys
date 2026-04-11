#pragma once

// Generic SPI ILI9341 240×320 → 320×240 landscape device profile.
//
// Pre-fills a device_profile for a typical 2.4" ILI9341 SPI breakout in
// landscape. Init and timing live in the HAL; products only adjust pins,
// offsets, or orientation flags to match a specific module.
//
// Override any field after construction.

#include "blusys/app/platform_profile.hpp"

namespace blusys::app::profiles {

// Logical 320×240 landscape; native panel is 240×320 with swap_xy enabled.
inline device_profile ili9341_320x240()
{
    device_profile p{};

    p.lcd.driver         = BLUSYS_LCD_DRIVER_ILI9341;
    p.lcd.width          = 320;
    p.lcd.height         = 240;
    p.lcd.bits_per_pixel = 16;
    p.lcd.swap_xy        = true;
    p.lcd.mirror_x       = true;
    p.lcd.mirror_y       = false;
    p.lcd.bgr_order      = false;
    p.lcd.invert_color   = false;

    p.lcd.spi.bus      = 0;
    p.lcd.spi.pclk_hz  = 40000000;
    p.lcd.spi.x_offset = 0;
    p.lcd.spi.y_offset = 0;

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
#else
    p.lcd.spi.sclk_pin = -1;
    p.lcd.spi.mosi_pin = -1;
    p.lcd.spi.cs_pin   = -1;
    p.lcd.spi.dc_pin   = -1;
#endif

    p.lcd.spi.rst_pin = -1;
    p.lcd.spi.bl_pin  = -1;

    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 24;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_RGB565;

    p.has_encoder = false;
    p.brightness  = 100;

    return p;
}

}  // namespace blusys::app::profiles
