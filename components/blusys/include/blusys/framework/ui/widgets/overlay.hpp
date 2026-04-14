#pragma once

#include <cstdint>

#include "blusys/framework/callbacks.hpp"
#include "lvgl.h"

// `bu_overlay` — themed transient feedback surface (toast-style).
//
// Visual intent: a compact bottom-anchored bubble using `color_primary`
// as its background and `color_on_primary` for the text label, padded
// with `spacing_md` and rounded with `radius_button`. Width grows with
// content up to a maximum of 80 % of the parent. Less intrusive than a
// `bu_modal` — there is no backdrop dim, the overlay simply pops over
// the existing content.
//
// Overlays are created **hidden** by convention. Calling `overlay_show`
// un-hides the overlay, brings it to the front of its parent, and (if
// `duration_ms > 0`) starts a one-shot LVGL timer that hides the
// overlay automatically when the timer elapses. Setting `duration_ms`
// to 0 disables the auto-dismiss — the product must call `overlay_hide`
// explicitly.
//
// The optional `on_hidden` callback fires whenever the overlay
// transitions from visible to hidden, regardless of whether the
// transition was caused by the auto-dismiss timer or by an explicit
// `overlay_hide`. (It does **not** fire if the overlay is already
// hidden when `overlay_hide` is called.)
//
// The returned `lv_obj_t *` is the overlay container and is the
// opaque handle. Use the `overlay_*` functions for state changes.

namespace blusys {

struct overlay_config {
    const char *text        = nullptr;
    uint32_t    duration_ms = 2500;  // 0 = manual dismiss only
    press_cb_t  on_hidden   = nullptr;
    void       *user_data   = nullptr;
};

lv_obj_t *overlay_create(lv_obj_t *parent, const overlay_config &config);

void overlay_show(lv_obj_t *overlay);
void overlay_hide(lv_obj_t *overlay);
bool overlay_is_visible(lv_obj_t *overlay);

}  // namespace blusys
