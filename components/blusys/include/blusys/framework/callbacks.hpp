#pragma once

#include <cstdint>

namespace blusys {

// Semantic callback types used by interactive widgets. Widgets accept these
// in their `<name>_config` structs and translate LVGL events into them
// internally — product code never sees `lv_event_cb_t` or `lv_event_t`.
//
// All callbacks are plain function pointers. Captureless lambdas decay to
// function pointers via the `+[]` idiom; lambdas with captures are not
// supported. Pass state through `user_data`.

using press_cb_t      = void (*)(void *user_data);
using release_cb_t    = void (*)(void *user_data);
using long_press_cb_t = void (*)(void *user_data);

using toggle_cb_t = void (*)(bool new_state, void *user_data);
using change_cb_t = void (*)(int32_t new_value, void *user_data);

// Discrete selection from a set of items (list, dropdown, tabs, data_table).
using select_cb_t = void (*)(int32_t index, void *user_data);

// Text submission (input_field).
using text_cb_t = void (*)(const char *text, void *user_data);

}  // namespace blusys
