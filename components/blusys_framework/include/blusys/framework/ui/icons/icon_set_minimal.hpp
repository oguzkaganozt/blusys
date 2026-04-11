#pragma once

#include "blusys/framework/ui/icons/icon_set.hpp"

namespace blusys::ui {

// Minimal built-in icon set using LVGL's LV_SYMBOL_* glyphs.
// Zero binary size cost — these glyphs are already in the default font.
// Products that need custom icons supply their own icon_set instead.
const icon_set &icon_set_minimal();

}  // namespace blusys::ui
