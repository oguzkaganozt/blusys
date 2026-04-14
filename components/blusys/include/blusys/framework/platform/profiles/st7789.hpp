#pragma once

// Generic SPI ST7789 240×320 → 320×240 landscape device profile.
//
// Pre-fills a device_profile for a common 2.0” ST7789 SPI module in
// landscape (320 wide × 240 tall after swap_xy). Pin defaults follow the
// same target-gated pattern as st7735.hpp for consistency across dev kits.
//
// Override any field after construction (offsets, mirrors, pins, backlight).
//
// Hardware validation (before treating defaults as board-specific “recommended”):
// - Image offset: set lcd.spi.x_offset / y_offset if the panel shows a colored
//   bar or shifted viewport (common on bare modules).
// - Inversion / color: toggle lcd.invert_color or adjust BGR if red/blue swap.
// - Backlight: set lcd.spi.bl_pin and drive brightness if your PCB wires BL.
// - SPI clock: lower pclk_hz if you see tearing or bus errors on long wires.

#include “blusys/framework/platform/profile.hpp”
#include “blusys/drivers/panels/st7789.h”

namespace blusys::platform {

// Logical 320×240 landscape; native panel is 240×320 with swap_xy enabled.
inline device_profile st7789_320x240()
{
    device_profile p{};
    p.lcd = blusys_st7789_default_config();

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

}  // namespace blusys::platform
