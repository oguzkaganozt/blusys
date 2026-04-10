#pragma once

#include <cstdint>

#include "lvgl.h"

// `bu_progress` — a themed horizontal progress bar.
//
// Visual intent: a track-and-indicator bar that displays progress from
// `min` to `max`. Optionally shows a percentage label on the bar and/or
// a descriptive text label below it.
//
// This is a display-only widget — no user interaction callbacks. State
// changes go through `progress_set_*` setters exclusively.

namespace blusys::ui {

struct progress_config {
    int32_t     min      = 0;
    int32_t     max      = 100;
    int32_t     initial  = 0;
    const char *label    = nullptr;   // optional text below bar
    bool        show_pct = false;     // show "XX%" text on bar
};

lv_obj_t *progress_create(lv_obj_t *parent, const progress_config &config);

void    progress_set_value(lv_obj_t *progress, int32_t value);
int32_t progress_get_value(lv_obj_t *progress);
void    progress_set_range(lv_obj_t *progress, int32_t min, int32_t max);
void    progress_set_label(lv_obj_t *progress, const char *text);

}  // namespace blusys::ui
