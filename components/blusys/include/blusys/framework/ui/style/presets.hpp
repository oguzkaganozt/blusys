#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/theme.hpp"

#include <cstdint>

namespace blusys::presets {

// Mirror of blusys_display_panel_kind_t — values must stay in sync with the
// driver enum.  Defined here so the framework UI layer does not have to
// include a driver header.
enum class panel_kind : std::uint8_t {
    rgb565        = 0,  // BLUSYS_DISPLAY_PANEL_KIND_RGB565
    mono_page     = 1,  // BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE
    rgb565_native = 2,  // BLUSYS_DISPLAY_PANEL_KIND_RGB565_NATIVE
};

// Expressive dark — saturated, tactile, characterful.
// Default for consumer/controller interactive products.
const blusys::theme_tokens &expressive_dark();

// Operational light — clean, readable, professionally restrained.
// Default for industrial/panel interactive products.
const blusys::theme_tokens &operational_light();

// Compact dark — dark, compact, operationally restrained.
// Small color-display variant for dense status surfaces.
const blusys::theme_tokens &compact_dark();

// OLED — pure white-on-black, zero radii, ultra-compact.
// Display-specific variant for monochrome or OLED panels.
const blusys::theme_tokens &oled();

// Scale a developer-supplied base theme for the given display dimensions.
// Preserves all colours and non-pixel fields; adjusts spacing, radius,
// shadow, touch_target_min, fonts, density_mode, and orientation.
// Returns a scaled copy — does not mutate `base`.
//
// Pass `profile.ui.panel_kind` as `kind`.
// When kind == panel_kind::mono_page the function returns
// `base` unchanged (OLED path is excluded from auto-scaling).
// When base.design_w == 0 || base.design_h == 0 the function returns
// `base` unchanged (safe no-op for uninitialised custom themes).
blusys::theme_tokens for_display(const blusys::theme_tokens &base,
                                  std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  panel_kind kind);

// Auto-select the closest built-in base preset for the given display, then
// scale it. Equivalent to for_display(auto_base(w, h, kind), w, h, kind).
//
// Base selection:
//   mono_page        → oled()
//   short-edge ≤ 200 → operational_light()
//   short-edge > 200 → expressive_dark()
blusys::theme_tokens for_display(std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  panel_kind kind);

}  // namespace blusys::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
