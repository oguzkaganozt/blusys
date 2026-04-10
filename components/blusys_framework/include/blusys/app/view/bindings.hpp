#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "lvgl.h"

#include <cstdint>

namespace blusys::app::view {

// Simple state binding helpers.
//
// These are thin wrappers over LVGL and blusys::ui setter functions.
// The value is namespace consistency: app code stays in blusys::app::view
// and never touches LVGL or blusys::ui directly for common state changes.
//
// Not reactive — products call these from update() when state changes.

// Label text
void set_text(lv_obj_t *label, const char *text);

// Slider value (does not fire on_change callback)
void set_value(lv_obj_t *slider, std::int32_t value);
std::int32_t get_value(lv_obj_t *slider);

// Slider range
void set_range(lv_obj_t *slider, std::int32_t min, std::int32_t max);

// Toggle state (does not fire on_change callback)
void set_checked(lv_obj_t *toggle, bool on);
bool get_checked(lv_obj_t *toggle);

// Enabled/disabled (uses blusys::ui widget setters when available)
void set_enabled(lv_obj_t *widget, bool enabled);

// Visibility
void set_visible(lv_obj_t *widget, bool visible);

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
