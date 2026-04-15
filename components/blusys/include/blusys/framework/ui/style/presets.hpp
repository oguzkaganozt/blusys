#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/drivers/display.h"

#include <cstdint>

namespace blusys::presets {

// Expressive dark — saturated, tactile, characterful.
// Default for consumer/controller interactive products.
const blusys::theme_tokens &expressive_dark();

// Operational light — clean, readable, professionally restrained.
// Default for industrial/panel interactive products.
const blusys::theme_tokens &operational_light();

// OLED — pure white-on-black, zero radii, ultra-compact.
// Display-specific variant for monochrome or OLED panels.
const blusys::theme_tokens &oled();

// Scale a developer-supplied base theme for the given display dimensions.
// Preserves all colours and non-pixel fields; adjusts spacing, radius,
// shadow, touch_target_min, fonts, density_mode, and orientation.
// Returns a scaled copy — does not mutate `base`.
//
// Pass `profile.ui.panel_kind` as `kind`.
// When kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE the function returns
// `base` unchanged (OLED path is excluded from auto-scaling).
// When base.design_w == 0 || base.design_h == 0 the function returns
// `base` unchanged (safe no-op for uninitialised custom themes).
blusys::theme_tokens for_display(const blusys::theme_tokens &base,
                                  std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys_display_panel_kind_t kind);

// Auto-select the closest built-in base preset for the given display, then
// scale it. Equivalent to for_display(auto_base(w, h, kind), w, h, kind).
//
// Base selection:
//   MONO_PAGE        → oled()
//   short-edge ≤ 200 → operational_light()
//   short-edge > 200 → expressive_dark()
blusys::theme_tokens for_display(std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys_display_panel_kind_t kind);

}  // namespace blusys::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
