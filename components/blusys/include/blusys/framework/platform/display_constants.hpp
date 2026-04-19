#pragma once

// Compile-time display tier helpers for if constexpr structural layout branching.
//
// Usage (BLUSYS_APP builds):
//
//   #include "blusys/framework/platform/display_constants.hpp"
//
//   if constexpr (blusys::kDisplayTier == blusys::display_tier::compact) {
//       // compact layout: single column, no sidebar, abbreviated labels
//   } else {
//       // full layout: multi-column, full labels
//   }
//
// kDisplayWidth, kDisplayHeight, and kDisplayTier are sourced from
// dashboard_display_dims.hpp (single source for the Kconfig/host dispatch).
//
// For BLUSYS_APP with a custom profile, use tier_for(w, h) directly:
//   if constexpr (blusys::tier_for(MY_W, MY_H) == blusys::display_tier::compact)

#include "blusys/framework/platform/dashboard_display_dims.hpp"
#include "blusys/framework/ui/style/display_types.hpp"

#include <cstdint>

namespace blusys {

// Short-edge breakpoint (px) separating compact-tier displays from full-tier.
// Used by both tier_for() below and presets::auto_base() in presets.cpp.
// Change here and in auto_base() together (the value is duplicated because
// presets.cpp is UI layer and cannot include this platform header without a
// layer violation).
inline constexpr std::uint32_t kTierBreakpointPx = 200;

// Two-tier model for color displays:
//   compact — short-edge <= kTierBreakpointPx (ST7735 160x128 and smaller)
//   full    — short-edge >  kTierBreakpointPx (ILI9341 320x240, ILI9488 480x320, etc.)
//
// OLED / mono displays are excluded from this tier model entirely;
// they use the oled() preset and the raw-LVGL path.
enum class display_tier : std::uint8_t {
    compact,
    full,
};

// Returns the display_tier for any pixel dimension pair.
// Usable in constexpr contexts for any profile.
constexpr display_tier tier_for(std::uint32_t w, std::uint32_t h) noexcept
{
    return ((w < h ? w : h) <= kTierBreakpointPx) ? display_tier::compact
                                                   : display_tier::full;
}

// Compile-time dashboard dimensions (from dashboard_display_dims.hpp).
inline constexpr std::uint32_t kDisplayWidth  = BLUSYS_DASHBOARD_DISPLAY_W;
inline constexpr std::uint32_t kDisplayHeight = BLUSYS_DASHBOARD_DISPLAY_H;

// Compile-time tier for if constexpr layout branching.
inline constexpr display_tier kDisplayTier = tier_for(kDisplayWidth, kDisplayHeight);

// Compile-time orientation for if constexpr portrait/landscape branching.
// Reflects the CONFIGURED dashboard profile (same source as kDisplayWidth/Height).
// To add a portrait profile, extend dashboard_display_dims.hpp with the new
// Kconfig/host macro pair — kDisplayOrientation updates automatically.
inline constexpr display_orientation kDisplayOrientation =
    orientation_for(kDisplayWidth, kDisplayHeight);

}  // namespace blusys
