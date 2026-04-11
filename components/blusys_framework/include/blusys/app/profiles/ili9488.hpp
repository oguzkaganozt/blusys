#pragma once

// Generic SPI ILI9488 320×480 → 480×320 landscape device profile.
//
// Large panels are memory- and bandwidth-sensitive; this profile uses a
// conservative SPI clock and slightly taller draw buffer lines. Validate
// flicker-free operation and DMA sizing on your hardware before shipping.
//
// Override any field after construction.

#include "blusys/app/platform_profile.hpp"

namespace blusys::app::profiles {

// Logical 480×320 landscape; native panel is 320×480 with swap_xy enabled.
inline device_profile ili9488_480x320()
{
    device_profile p{};

    p.lcd.driver         = BLUSYS_LCD_DRIVER_ILI9488;
    p.lcd.width          = 480;
    p.lcd.height         = 320;
    p.lcd.bits_per_pixel = 16;
    p.lcd.swap_xy        = true;
    p.lcd.mirror_x       = true;
    p.lcd.mirror_y       = false;
    p.lcd.bgr_order      = false;
    p.lcd.invert_color   = false;

    p.lcd.spi.bus      = 0;
    p.lcd.spi.pclk_hz  = 26000000;
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
    p.ui.buf_lines    = 32;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_RGB565;

    p.has_encoder = false;
    p.brightness  = 100;

    return p;
}

}  // namespace blusys::app::profiles
