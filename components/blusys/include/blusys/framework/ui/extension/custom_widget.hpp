#pragma once

// Custom Widget Contract for blusys::app products.
//
// Product-owned widgets that extend the view layer must follow this
// structural contract. It is enforced by code review, not inheritance.
//
// === The Six Rules ===
//
// 1. PUBLIC CONFIG STRUCT
//    The widget interface is a config/props struct with designated
//    initializers. Products never call raw LVGL creation functions.
//
// 2. RAW LVGL INSIDE ONLY
//    The widget implementation uses LVGL internally. Products interact
//    only through the config struct, create function, and setters.
//
// 3. THEME TOKENS ONLY
//    All visual values (colors, spacing, radii, fonts) come from
//    blusys::theme(). No magic numbers in widget implementations.
//
// 4. SETTERS OWN STATE TRANSITIONS
//    Products call widget_set_*() to change state. Setters never fire
//    callbacks (setter discipline). Products must not call
//    lv_obj_add_state/lv_obj_clear_state directly.
//
// 5. SEMANTIC OUTPUT
//    Widget behavior emits semantic callbacks (press_cb_t, etc.) or
//    dispatches app actions via app_ctx. No raw LVGL event exposure.
//
// 6. STANDARD FOCUS AND DISABLED
//    Interactive widgets support the standard focus group model and
//    disabled state. Hidden widgets are skipped by focus traversal.
//
// === Pattern ===
//
//   namespace my_product {
//
//   struct gauge_config {
//       float       min_value  = 0.0f;
//       float       max_value  = 100.0f;
//       float       initial    = 0.0f;
//       const char *unit_label = nullptr;
//   };
//
//   lv_obj_t *gauge_create(lv_obj_t *parent, const gauge_config &config);
//   void gauge_set_value(lv_obj_t *gauge, float value);
//   float gauge_get_value(lv_obj_t *gauge);
//   void gauge_set_disabled(lv_obj_t *gauge, bool disabled);
//
//   }  // namespace my_product
//
// No base class or tag is required. The contract is structural.

namespace blusys {

// Marker tag — products may inherit from this to document compliance.
// This is purely advisory and has no runtime effect.
struct custom_widget_tag {};

}  // namespace blusys
