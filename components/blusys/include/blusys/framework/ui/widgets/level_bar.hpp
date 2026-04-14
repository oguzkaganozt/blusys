#pragma once

#include <cstdint>

#include "lvgl.h"

// `bu_level_bar` — a compact horizontal level meter for audio-style UIs.
//
// Visual intent: a thin track with an accent-colored indicator. Display-only;
// updates go through `level_bar_set_*` setters. Differs from `bu_progress`
// primarily in default sizing and accent styling for channel / mix feedback.

namespace blusys {

struct level_bar_config {
    int32_t     min     = 0;
    int32_t     max     = 100;
    int32_t     initial = 0;
    const char *label   = nullptr;  // optional caption below the bar
};

lv_obj_t *level_bar_create(lv_obj_t *parent, const level_bar_config &config);

void    level_bar_set_value(lv_obj_t *level_bar, int32_t value);
int32_t level_bar_get_value(lv_obj_t *level_bar);
void    level_bar_set_range(lv_obj_t *level_bar, int32_t min, int32_t max);
void    level_bar_set_label(lv_obj_t *level_bar, const char *text);

}  // namespace blusys
