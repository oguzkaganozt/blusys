#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/theme.hpp"

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

}  // namespace blusys::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
