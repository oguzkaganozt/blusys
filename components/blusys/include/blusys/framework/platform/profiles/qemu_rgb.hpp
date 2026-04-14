#pragma once

// QEMU virtual framebuffer (esp_lcd_qemu_rgb) — same logical size as ILI9341 landscape.

#include "blusys/framework/platform/profile.hpp"
#include "blusys/drivers/panels/qemu_rgb.h"

namespace blusys::platform {

inline device_profile qemu_rgb_dashboard_320x240()
{
    device_profile p{};
    p.lcd = blusys_qemu_rgb_default_config();

    p.ui.lcd          = nullptr;
    p.ui.buf_lines    = 24;
    p.ui.full_refresh = false;
    p.ui.panel_kind   = BLUSYS_UI_PANEL_KIND_RGB565_NATIVE;

    p.has_encoder = false;
    p.brightness  = -1;

    return p;
}

}  // namespace blusys::platform
