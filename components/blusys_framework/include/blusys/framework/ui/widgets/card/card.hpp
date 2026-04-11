#pragma once

#include "lvgl.h"

// `bu_card` — an elevated container surface.
//
// Visual intent: a rounded, optionally shadowed container with padding
// and a vertical flex layout. Products add children into the card.
// Optionally shows a title label at the top.
//
// This is a layout widget — no user interaction callbacks. Used as a
// visual grouping surface by list items, settings sections, and
// dashboard panels.

namespace blusys::ui {

struct card_config {
    const char *title   = nullptr;   // optional title at top
    int         padding = -1;        // -1 = theme spacing_md
};

lv_obj_t *card_create(lv_obj_t *parent, const card_config &config);

void card_set_title(lv_obj_t *card, const char *title);

}  // namespace blusys::ui
