#pragma once

#include <cstdint>

namespace blusys {

// Physical orientation of the display as seen by the UI coordinate system.
// Computed by presets::for_display() and stored in theme_tokens::orientation.
// Also exposed as the compile-time constant blusys::kDisplayOrientation in
// display_constants.hpp for if constexpr structural layout branching.
//
// Zero-init value is landscape — the majority of TFT panels ship in default
// landscape configuration. Direct callers that bypass for_display() receive
// landscape without having to set the field explicitly.
enum class display_orientation : std::uint8_t {
    landscape,  // w >= h  (zero-init default)
    portrait,   // h > w
};

// Returns the display_orientation for any pixel dimension pair.
// Usable in constexpr contexts.
constexpr display_orientation orientation_for(std::uint32_t w,
                                               std::uint32_t h) noexcept
{
    return (h > w) ? display_orientation::portrait
                   : display_orientation::landscape;
}

}  // namespace blusys
