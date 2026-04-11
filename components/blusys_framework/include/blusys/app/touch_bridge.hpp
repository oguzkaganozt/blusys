#pragma once

// Touch input bridge — translates LVGL pointer events into framework intents.
//
// The touch bridge wraps an existing LVGL pointer indev (e.g., SDL mouse
// on host, or a capacitive touch driver on device) and posts semantic
// intents into the framework runtime:
//
//   tap         → intent::confirm
//   long-press  → intent::long_press
//   swipe left  → intent::focus_next
//   swipe right → intent::focus_prev
//
// Product code never touches this directly — the device entry path
// creates the bridge when the device_profile has has_touch == true.
// The host entry path creates it wrapping the SDL mouse indev.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/error.h"
#include "blusys/framework/core/runtime.hpp"

#include "lvgl.h"

namespace blusys::app {

struct touch_bridge_config {
    blusys::framework::runtime *framework_runtime = nullptr;
    bool                        enable_gestures   = true;
};

struct touch_bridge {
    lv_indev_t *indev = nullptr;
};

// Register an existing LVGL pointer indev as a touch source.
// Posts framework intents for tap, long-press, and swipe gestures.
blusys_err_t touch_bridge_open(const touch_bridge_config &config,
                                lv_indev_t *pointer_indev,
                                touch_bridge *out);

void touch_bridge_close(touch_bridge *bridge);

}  // namespace blusys::app

#endif  // BLUSYS_FRAMEWORK_HAS_UI
