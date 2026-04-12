// Headless LVGL display: proves partial flush + framebuffer path without SDL.
// Intended for CI and future pixel baselines (PNG compare can layer on top).

#include "lvgl.h"

#include <cstdlib>

namespace {

int g_flush_count = 0;

void stub_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    (void)px_map;
    ++g_flush_count;
    lv_display_flush_ready(disp);
}

}  // namespace

int main()
{
    lv_init();

    constexpr int32_t w         = 48;
    constexpr int32_t h         = 48;
    constexpr size_t  line_rows = 8;
    static lv_color_t buf[w * line_rows];

    lv_display_t *disp = lv_display_create(w, h);
    if (disp == nullptr) {
        return 1;
    }
    lv_display_set_buffers(disp, buf, nullptr, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, stub_flush);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x112233), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    // Partial mode may batch work across timer passes; enough iterations to force a flush.
    for (int i = 0; i < 64; ++i) {
        lv_timer_handler();
    }

    if (g_flush_count == 0) {
        return 2;
    }
    return 0;
}
