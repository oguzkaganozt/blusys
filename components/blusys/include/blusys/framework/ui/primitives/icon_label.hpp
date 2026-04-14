#pragma once

#include "lvgl.h"

// `bu_icon_label` — a compact icon + text pair.
//
// Visual intent: a semantic icon glyph next to a text label, arranged
// in a horizontal row. Used by status displays, list items, and
// navigation surfaces.
//
// Display-only primitive — no user interaction callbacks.

namespace blusys {

struct icon_label_config {
    const char      *icon      = nullptr;   // UTF-8 icon glyph string
    const char      *text      = nullptr;
    const lv_font_t *icon_font = nullptr;   // nullptr = theme icon font or font_body
    const lv_font_t *text_font = nullptr;   // nullptr = theme font_body
    lv_color_t       icon_color = {};
    bool             use_default_color = true;  // true = use color_on_surface
    int              gap       = -1;            // -1 = theme spacing_sm
};

lv_obj_t *icon_label_create(lv_obj_t *parent, const icon_label_config &config);

void icon_label_set_text(lv_obj_t *icon_label, const char *text);
void icon_label_set_icon(lv_obj_t *icon_label, const char *icon);

}  // namespace blusys
