#pragma once

#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

// `bu_modal` — themed modal dialog with a dimmed backdrop and centered card.
//
// Visual intent: a full-screen backdrop using `color_surface` at reduced
// opacity dims the underlying content; a centered card panel using
// `color_surface` at full opacity, bordered in `color_primary` with
// `radius_card` corners, holds an optional title (using `font_title`)
// and an optional body (using `font_body`). Layout is vertical flex
// inside the panel; the panel itself is centered inside the backdrop
// via flex alignment.
//
// Modals are created **hidden** by convention. Call `modal_show` to
// display, `modal_hide` to dismiss. The optional `on_dismiss` callback
// fires only when the user clicks the backdrop *outside* the panel —
// clicks on the panel are absorbed and do not dismiss. Products that
// want a "click anywhere to dismiss" behavior should leave on_dismiss
// nullptr and dismiss explicitly from another callsite.
//
// The returned `lv_obj_t *` is the backdrop container and is the
// opaque handle to the modal. Use the `modal_*` functions for state
// changes; do not call `lv_obj_*_flag` directly on the handle.

namespace blusys::ui {

struct modal_config {
    const char *title      = nullptr;
    const char *body       = nullptr;
    press_cb_t  on_dismiss = nullptr;
    void       *user_data  = nullptr;
};

lv_obj_t *modal_create(lv_obj_t *parent, const modal_config &config);

void modal_show(lv_obj_t *modal);
void modal_hide(lv_obj_t *modal);
bool modal_is_visible(lv_obj_t *modal);

}  // namespace blusys::ui
