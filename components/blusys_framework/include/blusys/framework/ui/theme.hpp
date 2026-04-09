#pragma once

#include "lvgl.h"

namespace blusys::ui {

struct theme_tokens {
    lv_color_t color_primary;
    lv_color_t color_surface;
    lv_color_t color_on_primary;
    lv_color_t color_accent;
    lv_color_t color_disabled;

    int spacing_sm;
    int spacing_md;
    int spacing_lg;

    int radius_card;
    int radius_button;

    const lv_font_t *font_body;
    const lv_font_t *font_title;
    const lv_font_t *font_mono;
};

void set_theme(const theme_tokens &tokens);
const theme_tokens &theme();

}  // namespace blusys::ui
