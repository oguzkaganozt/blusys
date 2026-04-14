#pragma once

#include <cstdint>

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

// `bu_slider` — themed continuous-value horizontal slider.
//
// Visual intent: a horizontal track with a filled indicator showing the
// current value and a draggable knob, styled by `theme()` tokens. The
// track uses `color_disabled`, the indicator uses `color_primary`, and
// the knob uses `color_on_primary`. Standard four states are supported
// via the stock `lv_slider` state machine.
//
// Construction takes a `slider_config` with explicit `min`, `max`, and
// `initial` values. The `on_change` callback is invoked with the new
// integer value whenever the user drags the slider. Programmatic
// updates via `slider_set_value` do **not** fire the callback —
// callbacks are reserved for user-driven transitions only, matching
// the convention established by `bu_button` and `bu_toggle`.
//
// Like every other interactive widget, the returned `lv_obj_t *` is
// an opaque handle. Use the `slider_set_*` setters for state changes;
// do not call `lv_slider_set_value` or `lv_obj_*_state` directly on
// the handle.

namespace blusys {

struct slider_config {
    int32_t     min       = 0;
    int32_t     max       = 100;
    int32_t     initial   = 0;
    change_cb_t on_change = nullptr;
    void       *user_data = nullptr;
    bool        disabled  = false;
};

lv_obj_t *slider_create(lv_obj_t *parent, const slider_config &config);

void    slider_set_value(lv_obj_t *slider, int32_t value);
int32_t slider_get_value(lv_obj_t *slider);
void    slider_set_range(lv_obj_t *slider, int32_t min, int32_t max);
void    slider_set_disabled(lv_obj_t *slider, bool disabled);

}  // namespace blusys
