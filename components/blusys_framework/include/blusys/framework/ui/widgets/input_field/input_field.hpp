#pragma once

#include <cstdint>

#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"

// `bu_input_field` — a themed text input field.
//
// Visual intent: a bordered text area with placeholder text, themed
// for surface and focus colors. Optionally masks input for passwords.
//
// Wraps `lv_textarea` with theme styling, callback slot pool, and
// setter discipline. `on_submit` fires on `LV_EVENT_READY` (enter/
// confirm). Programmatic `input_field_set_text` does not fire callbacks.

namespace blusys::ui {

struct input_field_config {
    const char  *placeholder = nullptr;
    const char  *initial     = nullptr;
    int32_t      max_length  = 64;
    bool         password    = false;
    text_cb_t    on_submit   = nullptr;
    void        *user_data   = nullptr;
    bool         disabled    = false;
};

lv_obj_t *input_field_create(lv_obj_t *parent, const input_field_config &config);

void        input_field_set_text(lv_obj_t *input, const char *text);
const char *input_field_get_text(lv_obj_t *input);
void        input_field_set_placeholder(lv_obj_t *input, const char *text);
void        input_field_set_disabled(lv_obj_t *input, bool disabled);

}  // namespace blusys::ui
