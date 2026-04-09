#pragma once

#include <cstdint>

#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

// `bu_button` — the platform's primary press-target widget.
//
// Visual intent: a themed, rounded button rendering a single text label,
// styled by `theme()` tokens and capable of four standard states
// (normal / focused / pressed / disabled). It is the flagship widget of the
// V1 widget kit and the template every other Camp 2 widget copies.
//
// Construction takes a `button_config` struct. Products never wire LVGL
// event callbacks directly: they pass `on_press` (a `press_cb_t`) and the
// widget translates `LV_EVENT_CLICKED` into that semantic callback.
//
// State transitions go through `button_set_*` setters. Products must not
// call `lv_obj_add_state`, `lv_obj_clear_state`, or style functions on the
// returned handle directly — that's the contract that makes Camp 2 / Camp 3
// implementations interchangeable.
//
// `lv_obj_t *` appears here only as an opaque handle returned to product
// code; it is not an invitation to drop back to raw LVGL.

namespace blusys::ui {

enum class button_variant : uint8_t {
    primary,
    secondary,
    ghost,
    danger,
};

struct button_config {
    const char    *label     = nullptr;
    button_variant variant   = button_variant::primary;
    press_cb_t     on_press  = nullptr;
    void          *user_data = nullptr;
    bool           disabled  = false;
};

lv_obj_t *button_create(lv_obj_t *parent, const button_config &config);

void button_set_label(lv_obj_t *button, const char *label);
void button_set_variant(lv_obj_t *button, button_variant variant);
void button_set_disabled(lv_obj_t *button, bool disabled);

}  // namespace blusys::ui
