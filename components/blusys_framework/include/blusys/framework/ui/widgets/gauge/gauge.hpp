#pragma once

#include <cstdint>

#include "lvgl.h"

// `bu_gauge` — a themed arc/ring progress display.
//
// Visual intent: an arc indicator showing a value within a range, with
// an optional centered value label and unit. Configurable start/end
// angles support 270-degree arcs, half-circles, VU meters, and ring
// indicators.
//
// Display-only widget — no user interaction callbacks, no slot pool.
// For interactive rotary control, use `knob` instead.

namespace blusys::ui {

struct gauge_config {
    int32_t     min         = 0;
    int32_t     max         = 100;
    int32_t     initial     = 0;
    int32_t     start_angle = 135;    // degrees, 0 = 3 o'clock
    int32_t     end_angle   = 405;    // 270-degree arc by default
    const char *unit        = nullptr; // e.g., "%", "dB", "RPM"
    bool        show_value  = true;
};

lv_obj_t *gauge_create(lv_obj_t *parent, const gauge_config &config);

void    gauge_set_value(lv_obj_t *gauge, int32_t value);
int32_t gauge_get_value(lv_obj_t *gauge);
void    gauge_set_range(lv_obj_t *gauge, int32_t min, int32_t max);
void    gauge_set_unit(lv_obj_t *gauge, const char *unit);

}  // namespace blusys::ui
