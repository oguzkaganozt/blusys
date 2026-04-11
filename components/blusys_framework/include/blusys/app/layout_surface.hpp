#pragma once

// Small layout and packaging hints derived from display geometry and panel
// kind — not a responsive layout engine. Use with theme tokens and shell
// config so one product can adjust density when the profile changes.

#include "blusys/app/platform_profile.hpp"

#include <cstdint>

namespace blusys::app::layout {

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

// Suggested shell chrome density — maps cleanly to view::shell_config toggles.
enum class shell_density : std::uint8_t {
    minimal,   // hide or shrink header/tabs on tiny surfaces
    standard,
    full,      // header + status + tabs where archetype expects it
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
                              blusys_ui_panel_kind_t panel_kind,
                              unsigned bits_per_pixel)
{
    surface_hints h{};
    h.is_landscape = width >= height;

    if (panel_kind == BLUSYS_UI_PANEL_KIND_MONO_PAGE || bits_per_pixel == 1u) {
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

}  // namespace blusys::app::layout
