#pragma once

#include "lvgl.h"

#include <cstdint>

namespace blusys {

// Forward declaration — defined in icons/icon_set.hpp.
struct icon_set;

// Density presets for spacing, touch targets, and information density.
enum class density : std::uint8_t {
    compact,      // industrial panels, small displays, dense status surfaces
    normal,       // default — balanced for most products
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

    // ---- density ----
    density density_mode;

    // Default feedback voice when identity.feedback is nullptr (custom themes
    // should set this to match their tactile intent).
    theme_feedback_voice feedback_voice;

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
