#pragma once

// Generic SPI ST7735 160x128 device profile.
//
// Returns a device_profile pre-filled for the canonical SPI ST7735
// panel in landscape orientation (160 wide x 128 tall after swap_xy).
// Pin defaults are target-gated to match common dev-board wiring.
//
// Products should override any field after construction:
//
//   auto profile = blusys::platform::st7735_160x128();
//   profile.lcd.spi.bl_pin = 4;  // product-specific backlight pin
//   BLUSYS_APP(spec)

#include "blusys/framework/platform/profile.hpp"
#include "blusys/drivers/panels/st7735.h"

namespace blusys::platform {

// Returns a device_profile for a generic 160x128 ST7735 SPI panel.
// Pin assignments use target-gated defaults matching common dev boards.
inline device_profile st7735_160x128()
{
    device_profile p{};
    p.lcd = blusys_st7735_default_config();

    p.lcd.spi.bus = 0;  // SPI2_HOST

    // Target-gated pin defaults
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

    // ---- UI config ----
    // ui.lcd is left null — filled by the device entry path after lcd_open().
    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 20;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_DISPLAY_PANEL_KIND_RGB565;

    // ---- defaults ----
    p.has_encoder = false;
    p.brightness  = 100;

    return p;
}

// Native portrait — 128×160.  swap_xy and mirror_x disabled.
// x_offset/y_offset are inherited from the landscape config; many 1.8"
// modules need these even in portrait — override if your module differs.
inline device_profile st7735_128x160()
{
    device_profile p = st7735_160x128();
    p.lcd.width    = BLUSYS_ST7735_NATIVE_WIDTH;
    p.lcd.height   = BLUSYS_ST7735_NATIVE_HEIGHT;
    p.lcd.swap_xy  = false;
    p.lcd.mirror_x = false;
    return p;
}

}  // namespace blusys::platform
