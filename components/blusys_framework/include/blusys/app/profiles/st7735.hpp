#pragma once

// Generic SPI ST7735 160x128 device profile.
//
// Returns a device_profile pre-filled for the canonical SPI ST7735
// panel in landscape orientation (160 wide x 128 tall after swap_xy).
// Pin defaults are target-gated to match common dev-board wiring.
//
// Products should override any field after construction:
//
//   auto profile = blusys::app::profiles::st7735_160x128();
//   profile.lcd.spi.bl_pin = 4;  // product-specific backlight pin
//   BLUSYS_APP_MAIN_DEVICE(spec, profile)

#include "blusys/app/platform_profile.hpp"

namespace blusys::app::profiles {

// Returns a device_profile for a generic 160x128 ST7735 SPI panel.
// Pin assignments use target-gated defaults matching common dev boards.
inline device_profile st7735_160x128()
{
    device_profile p{};

    // ---- LCD config ----
    p.lcd.driver         = BLUSYS_LCD_DRIVER_ST7735;
    p.lcd.width          = 160;
    p.lcd.height         = 128;
    p.lcd.bits_per_pixel = 16;
    p.lcd.swap_xy        = true;
    p.lcd.mirror_x       = true;
    p.lcd.mirror_y       = false;
    p.lcd.bgr_order      = false;
    p.lcd.invert_color   = false;

    p.lcd.spi.bus     = 0;  // SPI2_HOST
    p.lcd.spi.pclk_hz = 16000000;
    p.lcd.spi.x_offset = 2;
    p.lcd.spi.y_offset = 1;

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
#else
    // Unknown target — caller must set pins explicitly.
    p.lcd.spi.sclk_pin = -1;
    p.lcd.spi.mosi_pin = -1;
    p.lcd.spi.cs_pin   = -1;
    p.lcd.spi.dc_pin   = -1;
#endif

    p.lcd.spi.rst_pin = -1;
    p.lcd.spi.bl_pin  = -1;

    // ---- UI config ----
    // ui.lcd is left null — filled by the device entry path after lcd_open().
    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 20;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_RGB565;

    // ---- defaults ----
    p.has_encoder = false;
    p.brightness  = 100;

    return p;
}

}  // namespace blusys::app::profiles
