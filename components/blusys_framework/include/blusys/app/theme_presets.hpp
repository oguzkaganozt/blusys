#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/theme.hpp"

namespace blusys::app::presets {

// Dark theme — default for interactive products.
const blusys::ui::theme_tokens &dark();

// OLED theme — white-on-black, zero radii, compact spacing.
const blusys::ui::theme_tokens &oled();

// Light theme — daylight-readable, inverted dark palette.
const blusys::ui::theme_tokens &light();

}  // namespace blusys::app::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
