#pragma once

#include "lvgl.h"

// `bu_key_value` — a horizontal key-value text pair.
//
// Visual intent: a dimmed key label on the left, a normal-weight value
// on the right, in a single row. Used for device-info screens, settings
// displays, and diagnostic surfaces.
//
// Display-only primitive — no user interaction callbacks.

namespace blusys::ui {

struct key_value_config {
    const char *key   = nullptr;
    const char *value = nullptr;
};

lv_obj_t *key_value_create(lv_obj_t *parent, const key_value_config &config);

void key_value_set_key(lv_obj_t *kv, const char *key);
void key_value_set_value(lv_obj_t *kv, const char *value);

}  // namespace blusys::ui
