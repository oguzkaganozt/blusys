#pragma once

#include "lvgl.h"

#include <cstdint>

namespace blusys::ui {

// An icon set maps semantic icon roles to font glyph codepoints.
//
// Products that need custom icons provide their own icon_set with a
// custom font. The framework ships a minimal built-in set using
// LVGL's LV_SYMBOL_* glyphs (zero binary size cost).
//
// Glyph values are null-terminated UTF-8 strings (same format as
// LV_SYMBOL_* constants). nullptr means the icon is not available
// in this set — widgets must null-check before rendering.
struct icon_set {
    const lv_font_t *font;

    const char *wifi;
    const char *bluetooth;
    const char *ok;
    const char *warning;
    const char *error;
    const char *arrow_up;
    const char *arrow_down;
    const char *arrow_back;
    const char *settings;
    const char *info;
    const char *ota;
    const char *battery;
    const char *signal;
    const char *power;
    const char *refresh;
    const char *close;
};

}  // namespace blusys::ui
