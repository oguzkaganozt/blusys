#pragma once

// SSD1306 I2C OLED profiles (monochrome, page format via blusys_display).
//
// Defaults follow common breakout wiring; see examples/validation/lcd_ssd1306_basic
// Kconfig for the same per-target I2C pin choices. Use 128×64 for full-module
// status UIs; 128×32 for ultra-compact encoder or ticker-style surfaces.
//
// Override any field after construction.
//
// Hardware validation:
// - I2C address: many modules use 0x3C; some use 0x3D — set lcd.i2c.dev_addr.
// - Pins: defaults match examples/validation/lcd_ssd1306_basic per target;
//   override sda/scl for your PCB.
// - Some clones need lcd.invert_color = true for correct contrast.
// - SH1106-based modules are not the same controller; see docs/app/profiles.md
//   — use SSD1306 unless hardware requires a dedicated SH1106 profile.

#include "blusys/app/platform_profile.hpp"

#include <cstdint>

namespace blusys::app::profiles {

namespace detail {

inline void ssd1306_fill_common(device_profile &p, std::uint32_t width,
                                std::uint32_t height)
{
    p.lcd.driver         = BLUSYS_LCD_DRIVER_SSD1306;
    p.lcd.width          = width;
    p.lcd.height         = height;
    p.lcd.bits_per_pixel = 1;
    p.lcd.swap_xy        = false;
    p.lcd.mirror_x       = false;
    p.lcd.mirror_y       = false;
    p.lcd.bgr_order      = false;
    p.lcd.invert_color   = false;

    p.lcd.i2c.port     = 0;
    p.lcd.i2c.dev_addr = 0x3C;
    p.lcd.i2c.rst_pin  = -1;
    p.lcd.i2c.freq_hz  = 400000;

#if CONFIG_IDF_TARGET_ESP32
    p.lcd.i2c.sda_pin = 21;
    p.lcd.i2c.scl_pin = 22;
#elif CONFIG_IDF_TARGET_ESP32C3
    p.lcd.i2c.sda_pin = 5;
    p.lcd.i2c.scl_pin = 6;
#elif CONFIG_IDF_TARGET_ESP32S3
    p.lcd.i2c.sda_pin = 1;
    p.lcd.i2c.scl_pin = 2;
#else
    p.lcd.i2c.sda_pin = -1;
    p.lcd.i2c.scl_pin = -1;
#endif

    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 8;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_MONO_PAGE;

    p.has_encoder = false;
    p.brightness  = 100;
}

}  // namespace detail

// 128×64 — common 0.96" SSD1306 module.
inline device_profile ssd1306_128x64()
{
    device_profile p{};
    detail::ssd1306_fill_common(p, 128, 64);
    return p;
}

// 128×32 — compact modules (half-height).
inline device_profile ssd1306_128x32()
{
    device_profile p{};
    detail::ssd1306_fill_common(p, 128, 32);
    return p;
}

}  // namespace blusys::app::profiles
