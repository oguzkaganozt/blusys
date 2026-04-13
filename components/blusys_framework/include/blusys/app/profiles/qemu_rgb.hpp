#pragma once

// QEMU virtual framebuffer (esp_lcd_qemu_rgb) — same logical size as ILI9341 landscape.

#include "blusys/app/platform_profile.hpp"

namespace blusys::app::profiles {

inline device_profile qemu_rgb_dashboard_320x240()
{
    device_profile p{};

    p.lcd.driver         = BLUSYS_LCD_DRIVER_QEMU_RGB;
    p.lcd.width          = 320;
    p.lcd.height         = 240;
    p.lcd.bits_per_pixel = 16;
    p.lcd.swap_xy        = false;
    p.lcd.mirror_x       = false;
    p.lcd.mirror_y       = false;
    p.lcd.bgr_order      = false;
    p.lcd.invert_color   = false;

    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 24;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_RGB565_NATIVE;

    p.has_encoder = false;
    p.brightness  = -1;

    return p;
}

}  // namespace blusys::app::profiles
