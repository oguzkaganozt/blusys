#pragma once

#include <cstdint>

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

// `bu_knob` — an interactive rotary arc control (Camp 3: per-instance
// callback storage via LVGL allocator; no global slot pool).
//
// Visual intent: a draggable arc knob for setting a value within a
// range. Fires `change_cb_t` on user-driven value changes.
//
// Setter discipline: `knob_set_value` does not fire the callback.

namespace blusys {

struct knob_config {
    int32_t     min         = 0;
    int32_t     max         = 100;
    int32_t     initial     = 0;
    int32_t     start_angle = 135;
    int32_t     end_angle   = 405;
    change_cb_t on_change   = nullptr;
    void       *user_data   = nullptr;
    bool        disabled    = false;
};

lv_obj_t *knob_create(lv_obj_t *parent, const knob_config &config);

void    knob_set_value(lv_obj_t *knob, int32_t value);
int32_t knob_get_value(lv_obj_t *knob);
void    knob_set_range(lv_obj_t *knob, int32_t min, int32_t max);
void    knob_set_disabled(lv_obj_t *knob, bool disabled);

}  // namespace blusys
