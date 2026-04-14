#pragma once

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

// `bu_toggle` — themed two-state switch.
//
// Visual intent: a horizontal switch with a track and a sliding knob,
// styled by `theme()` tokens. The active (checked) track uses
// `color_primary`; the inactive track uses `color_disabled`; the knob
// uses `color_on_primary`. Standard four states are supported via the
// stock `lv_switch` state machine plus the `LV_STATE_CHECKED` state.
//
// Construction takes a `toggle_config`. The `on_change` callback is
// invoked with the new boolean state whenever the user flips the
// switch. Programmatic state changes via `toggle_set_state` do **not**
// fire the callback — the callback is reserved for user-driven
// transitions only.
//
// Like every other interactive widget, the returned `lv_obj_t *` is
// an opaque handle. Use the `toggle_set_*` setters for state changes;
// do not call `lv_obj_*_state` directly on the handle.

namespace blusys {

struct toggle_config {
    bool        initial   = false;
    toggle_cb_t on_change = nullptr;
    void       *user_data = nullptr;
    bool        disabled  = false;
};

lv_obj_t *toggle_create(lv_obj_t *parent, const toggle_config &config);

void toggle_set_state(lv_obj_t *toggle, bool on);
bool toggle_get_state(lv_obj_t *toggle);
void toggle_set_disabled(lv_obj_t *toggle, bool disabled);

}  // namespace blusys
