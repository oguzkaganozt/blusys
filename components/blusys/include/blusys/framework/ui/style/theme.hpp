#pragma once

#include "lvgl.h"

#include "blusys/framework/ui/style/display_types.hpp"

#include <cstdint>

namespace blusys {

// Forward declaration — defined in icons/icon_set.hpp.
struct icon_set;

// Density presets for spacing, touch targets, and information density.
enum class density : std::uint8_t {
    normal,       // default — balanced for most products (zero-init value)
    compact,      // industrial panels, small displays, dense status surfaces
    comfortable,  // consumer controllers, touch-first surfaces
};

// Which built-in feedback_preset family matches this theme when app_identity
// does not set identity.feedback (visual flash + default haptic alignment).
enum class theme_feedback_voice : std::uint8_t {
    expressive,
    operational,
};

struct theme_tokens {
    // ---- core palette ----
    lv_color_t color_primary;
    lv_color_t color_surface;
    lv_color_t color_on_primary;
    lv_color_t color_accent;
    lv_color_t color_disabled;

    // ---- semantic status colors ----
    lv_color_t color_success;
    lv_color_t color_warning;
    lv_color_t color_error;
    lv_color_t color_info;
    lv_color_t color_on_success;
    lv_color_t color_on_warning;
    lv_color_t color_on_error;

    // ---- surface and outline ----
    lv_color_t color_background;
    lv_color_t color_on_surface;
    // Elevated surfaces (cards, panels) distinct from flat background.
    lv_color_t color_surface_elevated;
    lv_color_t color_outline;
    lv_color_t color_outline_variant;

    // ---- display context (both fields computed by presets::for_display()) ----
    // Preset initializers do not set these; direct callers that bypass
    // for_display() receive the zero-init values (density::normal,
    // display_orientation::landscape). See ADR 0003.
    density            density_mode;
    display_orientation orientation;

    // Default feedback voice when identity.feedback is nullptr (custom themes
    // should set this to match their tactile intent).
    theme_feedback_voice feedback_voice;

    // ---- design resolution ----
    // The pixel dimensions this theme was authored for.
    // for_display() normalises both sides to (short_edge, long_edge) and
    // computes scale = min(short_actual/short_design, long_actual/long_design),
    // so a 320×240 preset produces ratio 1.0 on a 240×320 portrait display.
    // Must be set explicitly in every preset — designated initializer
    // aggregate-init zero-initialises fields not listed, so there is no
    // safe default. A zero value disables scaling (for_display() returns
    // the theme unchanged when design_w == 0 || design_h == 0).
    std::uint32_t design_w = 0;
    std::uint32_t design_h = 0;

    // ---- spacing ----
    int spacing_xs;
    int spacing_sm;
    int spacing_md;
    int spacing_lg;
    int spacing_xl;
    int spacing_2xl;

    int touch_target_min;  // minimum interactive target size (px)

    // ---- radii ----
    int radius_card;
    int radius_button;

    // ---- typography ----
    const lv_font_t *font_body;
    const lv_font_t *font_body_sm;
    const lv_font_t *font_title;
    const lv_font_t *font_display;
    const lv_font_t *font_label;
    const lv_font_t *font_mono;

    // Extra letter spacing for body styles (0 = LVGL default).
    lv_coord_t text_letter_space_body;

    // ---- motion ----
    std::uint16_t anim_duration_fast;      // ms
    std::uint16_t anim_duration_normal;    // ms
    std::uint16_t anim_duration_slow;      // ms
    lv_anim_path_cb_t anim_path_standard;
    lv_anim_path_cb_t anim_path_enter;
    lv_anim_path_cb_t anim_path_exit;
    bool anim_enabled;

    // ---- depth / elevation ----
    std::uint8_t shadow_card_width;
    std::int16_t shadow_card_ofs_y;
    std::uint8_t shadow_card_spread;
    lv_color_t   shadow_color;

    std::uint8_t shadow_overlay_width;
    std::int16_t shadow_overlay_ofs_y;

    // ---- interaction state ----
    lv_opa_t   opa_pressed;
    lv_opa_t   opa_focused;
    lv_opa_t   opa_disabled;
    lv_opa_t   opa_backdrop;
    // Muted fills (inactive tabs, chips).
    lv_opa_t   opa_surface_subtle;
    // Soft drop shadow on modals / floating overlays.
    lv_opa_t   opa_shadow_soft;
    int        focus_ring_width;
    lv_color_t color_focus_ring;

    // ---- icons (optional) ----
    const icon_set *icons;
};

void set_theme(const theme_tokens &tokens);
const theme_tokens &theme();

}  // namespace blusys
