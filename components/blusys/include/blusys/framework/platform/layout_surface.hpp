#pragma once

// Small layout and packaging hints derived from display geometry and panel
// kind — not a responsive layout engine. Use with theme tokens and shell
// config so one product can adjust density when the profile changes.
// For LVGL flex composition (shell, scroll viewport, row/col primitives), see
// docs/internals/architecture.md#ui-layout-lvgl-flex.
//
// Rotation and resolution:
// - Panel rotation (swap_xy, mirror_*) belongs in device_profile / HAL: the
//   logical width and height exposed here are already the post-transform size
//   the UI uses (e.g. 320×240 landscape after swap_xy on a 240×320 ST7789).
// - classify() uses those logical dimensions and bits_per_pixel; it does not
//   re-apply hardware rotation. Keep orientation fixes in the profile, not in
//   screens.

#include "blusys/framework/platform/profile.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/style/theme.hpp"
#endif

#include <cstdint>

namespace blusys::layout {

enum class surface_size : std::uint8_t {
    tiny_mono,  // SSD1306-style local status
    compact,    // ST7735 / small handheld
    medium,     // ILI9341 / ST7789 “dashboard” class
    large,      // ILI9488 and similar
};

enum class theme_packaging_hint : std::uint8_t {
    expressive,    // compact interactive / controller bias
    operational,     // panel / gateway local operator UI bias
    monochrome,      // OLED / mono — use presets::oled()
};

// Suggested shell chrome density — maps cleanly to shell_config toggles.
enum class shell_density : std::uint8_t {
    minimal,   // hide or shrink header/tabs on tiny surfaces
    standard,
    full,      // header + status + tabs where the shell expects them
};

struct surface_hints {
    surface_size          size_class       = surface_size::medium;
    bool                  is_landscape     = true;
    bool                  is_monochrome  = false;
    shell_density         shell            = shell_density::standard;
    int                   spacing_level    = 1;  // 0 = tight, 1 = normal, 2 = airy
    int                   typography_level = 1;  // 0 = smallest stock scale band
    theme_packaging_hint  theme_hint       = theme_packaging_hint::operational;
};

// Small shell toggles derived from the classified surface. This keeps
// product code from repeating raw size checks when selecting shell chrome.
struct shell_chrome {
    bool status_enabled = true;
    bool tabs_enabled   = true;
};

// Classify from pixel dimensions and UI panel kind (matches device_profile.ui).
inline surface_hints classify(std::uint32_t width, std::uint32_t height,
                              blusys_display_panel_kind_t panel_kind,
                              unsigned bits_per_pixel)
{
    surface_hints h{};
    h.is_landscape = width >= height;

    if (panel_kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE || bits_per_pixel == 1u) {
        h.size_class       = surface_size::tiny_mono;
        h.is_monochrome    = true;
        h.shell            = shell_density::minimal;
        h.spacing_level    = 0;
        h.typography_level = 0;
        h.theme_hint       = theme_packaging_hint::monochrome;
        return h;
    }

    const std::uint32_t mx = width > height ? width : height;
    const std::uint32_t mn = width < height ? width : height;

    if (mx <= 200u && mn <= 180u) {
        h.size_class       = surface_size::compact;
        h.shell            = shell_density::standard;
        h.spacing_level    = 0;
        h.typography_level = 0;
        h.theme_hint       = theme_packaging_hint::expressive;
    } else if (mx <= 400u) {
        h.size_class       = surface_size::medium;
        h.shell            = shell_density::full;
        h.spacing_level    = 1;
        h.typography_level = 1;
        h.theme_hint       = theme_packaging_hint::operational;
    } else {
        h.size_class       = surface_size::large;
        h.shell            = shell_density::full;
        h.spacing_level    = 2;
        h.typography_level = 2;
        h.theme_hint       = theme_packaging_hint::operational;
    }

    return h;
}

inline surface_hints classify(const device_profile &profile)
{
    return classify(profile.lcd.width, profile.lcd.height, profile.ui.panel_kind,
                    profile.lcd.bits_per_pixel);
}

inline shell_chrome shell_chrome_for(const surface_hints &h)
{
    shell_chrome c{};
    c.status_enabled = h.shell != shell_density::minimal;
    c.tabs_enabled   = h.size_class != surface_size::tiny_mono;
    return c;
}

inline shell_chrome shell_chrome_for(const device_profile &profile)
{
    return shell_chrome_for(classify(profile));
}

#ifdef BLUSYS_FRAMEWORK_HAS_UI

// Maps packaging hints from classify() to a suggested theme_tokens::density_mode.
// Aligns with presets::expressive_dark() (comfortable), operational_light()
// (compact), compact_dark() (compact), and presets::oled() (compact).
// Products can copy this into a custom theme when the display profile changes
// at integration time.
inline blusys::density suggested_theme_density(const surface_hints &h)
{
    switch (h.theme_hint) {
    case theme_packaging_hint::expressive:
        return blusys::density::comfortable;
    case theme_packaging_hint::operational:
    case theme_packaging_hint::monochrome:
    default:
        return blusys::density::compact;
    }
}

inline blusys::density suggested_theme_density(const device_profile &profile)
{
    return suggested_theme_density(classify(profile));
}

#endif  // BLUSYS_FRAMEWORK_HAS_UI

}  // namespace blusys::layout
