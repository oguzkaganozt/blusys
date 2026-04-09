#pragma once

#include "lvgl.h"

// Encoder focus helpers — pure LVGL group/focus wiring.
//
// These helpers stay independent of the encoder *driver* (which lives
// in `blusys_services::input::encoder`). Products are responsible for
// creating their own `lv_indev_t` for the encoder hardware and binding
// it to the group returned by `create_encoder_group` via
// `lv_indev_set_group`. The framework only owns the LVGL-side wiring:
// the group itself and the rule for which widgets become focusable.
//
// Focusability rule: a widget is added to the focus group if it has
// `LV_OBJ_FLAG_CLICKABLE` set and is not inside a hidden subtree. The
// V1 widget kit's interactive widgets (`bu_button`, `bu_toggle`,
// `bu_slider`, and `bu_modal` when its dismiss callback is wired)
// satisfy this. Layout primitives, labels, and dividers are not
// clickable and are skipped.

namespace blusys::ui {

// Create an LVGL focus group suitable for use with an encoder input
// device. Returns a freshly-allocated group, or nullptr on failure.
//
// The caller owns the group and must destroy it via `lv_group_delete`
// when the screen is replaced or the product shuts down.
lv_group_t *create_encoder_group();

// Walk `screen` recursively and add every focusable descendant to
// `group` in DFS pre-order (parents before children, left-to-right).
// After the walk, focus is set to the first focusable widget so the
// encoder lands on a sensible default position.
//
// Composition widgets like `bu_modal` and `bu_overlay` whose root
// container is `LV_OBJ_FLAG_HIDDEN` are skipped entirely — their
// children are not enumerated. This keeps focus traversal scoped to
// the actually-visible content of the screen.
//
// Safe to call repeatedly. Each call only adds objects to the supplied
// group; it does not clear pre-existing members. Call
// `lv_group_remove_all_objs(group)` first if a clean re-walk is needed.
void auto_focus_screen(lv_obj_t *screen, lv_group_t *group);

}  // namespace blusys::ui
