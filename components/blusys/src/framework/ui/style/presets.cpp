#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/presets.hpp"

#include "blusys/framework/ui/style/fonts.hpp"
#include "blusys/framework/ui/style/icon_set_minimal.hpp"

#include "lvgl.h"

namespace blusys::presets {

const blusys::theme_tokens &expressive_dark()
{
    static const blusys::theme_tokens tokens{
        // core palette — saturated, vibrant
        .color_primary    = lv_color_hex(0x3B6FFF),
        .color_surface    = lv_color_hex(0x0D1017),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        .color_accent     = lv_color_hex(0x00E5A0),
        .color_disabled   = lv_color_hex(0x4A5068),

        // semantic status
        .color_success    = lv_color_hex(0x22C55E),
        .color_warning    = lv_color_hex(0xFBBF24),
        .color_error      = lv_color_hex(0xEF4444),
        .color_info       = lv_color_hex(0x60A5FA),
        .color_on_success = lv_color_hex(0xFFFFFF),
        .color_on_warning = lv_color_hex(0x1A1A1A),
        .color_on_error   = lv_color_hex(0xFFFFFF),

        // surface and outline
        .color_background       = lv_color_hex(0x080A0F),
        .color_on_surface       = lv_color_hex(0xE4E8F0),
        .color_surface_elevated = lv_color_hex(0x161C2A),
        .color_outline          = lv_color_hex(0x2D3240),
        .color_outline_variant  = lv_color_hex(0x1E222E),

        .feedback_voice = blusys::theme_feedback_voice::expressive,

        // design resolution — ILI9341 320×240 reference
        .design_w = 320,
        .design_h = 240,

        // spacing
        .spacing_xs  = 4,
        .spacing_sm  = 8,
        .spacing_md  = 12,
        .spacing_lg  = 20,
        .spacing_xl  = 28,
        .spacing_2xl = 36,

        .touch_target_min = 44,

        // radii — generous, pill buttons
        .radius_card   = 14,
        .radius_button = 999,

        // typography — larger display/title ramp when Montserrat is enabled
        .font_body    = blusys::fonts::montserrat_14(),
        .font_body_sm = blusys::fonts::montserrat_14(),
        .font_title   = blusys::fonts::montserrat_20(),
        .font_display = blusys::fonts::montserrat_20(),
        .font_label   = blusys::fonts::montserrat_14(),
        .font_mono    = blusys::fonts::montserrat_14(),

        .text_letter_space_body = 0,

        // motion — full, expressive
        .anim_duration_fast   = 100,
        .anim_duration_normal = 220,
        .anim_duration_slow   = 380,
        .anim_path_standard   = lv_anim_path_ease_in_out,
        .anim_path_enter      = lv_anim_path_ease_out,
        .anim_path_exit       = lv_anim_path_ease_in,
        .anim_enabled         = true,

        // depth — visible shadows for card and overlay depth
        .shadow_card_width  = 10,
        .shadow_card_ofs_y  = 5,
        .shadow_card_spread = 3,
        .shadow_color       = lv_color_hex(0x000000),

        .shadow_overlay_width = 20,
        .shadow_overlay_ofs_y = 10,

        // interaction state — strong tactile feel
        .opa_pressed      = LV_OPA_80,
        .opa_focused      = LV_OPA_50,
        .opa_disabled     = LV_OPA_30,
        .opa_backdrop         = LV_OPA_70,
        .opa_surface_subtle   = LV_OPA_20,
        .opa_shadow_soft      = LV_OPA_40,
        .focus_ring_width = 2,
        .color_focus_ring = lv_color_hex(0x3B6FFF),

        // icons
        .icons = &blusys::icon_set_minimal(),
    };
    return tokens;
}

const blusys::theme_tokens &operational_light()
{
    static const blusys::theme_tokens tokens{
        // core palette — muted, professional
        .color_primary    = lv_color_hex(0x1A56DB),
        .color_surface    = lv_color_hex(0xFFFFFF),
        .color_on_primary = lv_color_hex(0xFFFFFF),
        .color_accent     = lv_color_hex(0x0EA5E9),
        .color_disabled   = lv_color_hex(0xBDBDBD),

        // semantic status — high contrast for readability
        .color_success    = lv_color_hex(0x16A34A),
        .color_warning    = lv_color_hex(0xD97706),
        .color_error      = lv_color_hex(0xDC2626),
        .color_info       = lv_color_hex(0x2563EB),
        .color_on_success = lv_color_hex(0xFFFFFF),
        .color_on_warning = lv_color_hex(0xFFFFFF),
        .color_on_error   = lv_color_hex(0xFFFFFF),

        // surface and outline
        .color_background       = lv_color_hex(0xF4F6F9),
        .color_on_surface       = lv_color_hex(0x1F2937),
        .color_surface_elevated = lv_color_hex(0xFFFFFF),
        .color_outline          = lv_color_hex(0xD1D5DB),
        .color_outline_variant  = lv_color_hex(0xE5E7EB),

        .feedback_voice = blusys::theme_feedback_voice::operational,

        // design resolution — ST7735 160×128 reference
        .design_w = 160,
        .design_h = 128,

        // spacing — tighter
        .spacing_xs  = 2,
        .spacing_sm  = 4,
        .spacing_md  = 8,
        .spacing_lg  = 12,
        .spacing_xl  = 16,
        .spacing_2xl = 24,

        .touch_target_min = 36,

        // radii — subtle, professional
        .radius_card   = 8,
        .radius_button = 6,

        // typography — uniform, dense sans hierarchy (operational clarity)
        .font_body    = blusys::fonts::montserrat_14(),
        .font_body_sm = blusys::fonts::montserrat_14(),
        .font_title   = blusys::fonts::montserrat_14(),
        .font_display = blusys::fonts::montserrat_14(),
        .font_label   = blusys::fonts::montserrat_14(),
        .font_mono    = blusys::fonts::montserrat_14(),

        .text_letter_space_body = 1,

        // motion — reduced, snappy
        .anim_duration_fast   = 60,
        .anim_duration_normal = 120,
        .anim_duration_slow   = 200,
        .anim_path_standard   = lv_anim_path_ease_in_out,
        .anim_path_enter      = lv_anim_path_ease_out,
        .anim_path_exit       = lv_anim_path_ease_in,
        .anim_enabled         = false,

        // depth — flat, minimal shadow
        .shadow_card_width  = 2,
        .shadow_card_ofs_y  = 1,
        .shadow_card_spread = 0,
        .shadow_color       = lv_color_hex(0x9CA3AF),

        .shadow_overlay_width = 4,
        .shadow_overlay_ofs_y = 2,

        // interaction state — subtle
        .opa_pressed      = LV_OPA_90,
        .opa_focused      = LV_OPA_30,
        .opa_disabled     = LV_OPA_40,
        .opa_backdrop         = LV_OPA_60,
        .opa_surface_subtle   = LV_OPA_20,
        .opa_shadow_soft      = LV_OPA_30,
        .focus_ring_width = 2,
        .color_focus_ring = lv_color_hex(0x1A56DB),

        // icons
        .icons = &blusys::icon_set_minimal(),
    };
    return tokens;
}

const blusys::theme_tokens &compact_dark()
{
    static const blusys::theme_tokens tokens = [] {
        blusys::theme_tokens t = expressive_dark();

        // Compact layout tuned for small color displays.
        t.design_w = 160;
        t.design_h = 128;

        t.spacing_xs  = 2;
        t.spacing_sm  = 4;
        t.spacing_md  = 8;
        t.spacing_lg  = 12;
        t.spacing_xl  = 16;
        t.spacing_2xl = 24;

        t.touch_target_min = 36;

        t.radius_card   = 8;
        t.radius_button = 6;

        t.font_body    = blusys::fonts::montserrat_14();
        t.font_body_sm = blusys::fonts::montserrat_14();
        t.font_title   = blusys::fonts::montserrat_14();
        t.font_display = blusys::fonts::montserrat_14();
        t.font_label   = blusys::fonts::montserrat_14();
        t.font_mono    = blusys::fonts::montserrat_14();

        t.text_letter_space_body = 1;

        t.anim_duration_fast   = 60;
        t.anim_duration_normal = 120;
        t.anim_duration_slow   = 200;
        t.anim_enabled         = false;

        t.shadow_card_width  = 2;
        t.shadow_card_ofs_y  = 1;
        t.shadow_card_spread = 0;

        t.shadow_overlay_width = 4;
        t.shadow_overlay_ofs_y = 2;

        t.opa_pressed      = LV_OPA_90;
        t.opa_focused      = LV_OPA_30;
        t.opa_disabled     = LV_OPA_40;
        t.opa_backdrop     = LV_OPA_60;
        t.opa_surface_subtle = LV_OPA_20;
        t.opa_shadow_soft    = LV_OPA_30;
        t.focus_ring_width   = 2;
        t.color_focus_ring   = lv_color_hex(0x3B6FFF);

        t.feedback_voice = blusys::theme_feedback_voice::operational;
        return t;
    }();
    return tokens;
}

const blusys::theme_tokens &oled()
{
    static const blusys::theme_tokens tokens{
        // core palette — pure black/white
        .color_primary    = lv_color_hex(0xFFFFFF),
        .color_surface    = lv_color_hex(0x000000),
        .color_on_primary = lv_color_hex(0x000000),
        .color_accent     = lv_color_hex(0xFFFFFF),
        .color_disabled   = lv_color_hex(0x555555),

        // semantic status — monochrome-safe
        .color_success    = lv_color_hex(0xFFFFFF),
        .color_warning    = lv_color_hex(0xAAAAAA),
        .color_error      = lv_color_hex(0xFFFFFF),
        .color_info       = lv_color_hex(0xCCCCCC),
        .color_on_success = lv_color_hex(0x000000),
        .color_on_warning = lv_color_hex(0x000000),
        .color_on_error   = lv_color_hex(0x000000),

        // surface and outline
        .color_background       = lv_color_hex(0x000000),
        .color_on_surface       = lv_color_hex(0xFFFFFF),
        .color_surface_elevated = lv_color_hex(0x0A0A0A),
        .color_outline          = lv_color_hex(0x444444),
        .color_outline_variant  = lv_color_hex(0x333333),

        .feedback_voice = blusys::theme_feedback_voice::operational,

        // design resolution — SSD1306 128×64 reference; OLED bypasses scaling
        // but dimensions must be set to prevent division-by-zero if for_display()
        // is ever called with MONO_PAGE kind.
        .design_w = 128,
        .design_h = 64,

        // spacing — minimal
        .spacing_xs  = 1,
        .spacing_sm  = 2,
        .spacing_md  = 4,
        .spacing_lg  = 8,
        .spacing_xl  = 12,
        .spacing_2xl = 16,

        .touch_target_min = 28,

        // radii — zero for pixel-perfect OLED
        .radius_card   = 0,
        .radius_button = 0,

        // typography
        .font_body    = LV_FONT_DEFAULT,
        .font_body_sm = LV_FONT_DEFAULT,
        .font_title   = LV_FONT_DEFAULT,
        .font_display = LV_FONT_DEFAULT,
        .font_label   = LV_FONT_DEFAULT,
        .font_mono    = LV_FONT_DEFAULT,

        .text_letter_space_body = 0,

        // motion — off on tiny displays
        .anim_duration_fast   = 0,
        .anim_duration_normal = 0,
        .anim_duration_slow   = 0,
        .anim_path_standard   = lv_anim_path_ease_in_out,
        .anim_path_enter      = lv_anim_path_ease_out,
        .anim_path_exit       = lv_anim_path_ease_in,
        .anim_enabled         = false,

        // depth — none
        .shadow_card_width  = 0,
        .shadow_card_ofs_y  = 0,
        .shadow_card_spread = 0,
        .shadow_color       = lv_color_hex(0x000000),

        .shadow_overlay_width = 0,
        .shadow_overlay_ofs_y = 0,

        // interaction state
        .opa_pressed      = LV_OPA_70,
        .opa_focused      = LV_OPA_50,
        .opa_disabled     = LV_OPA_30,
        .opa_backdrop         = LV_OPA_80,
        .opa_surface_subtle   = LV_OPA_10,
        .opa_shadow_soft      = LV_OPA_30,
        .focus_ring_width = 1,
        .color_focus_ring = lv_color_hex(0xFFFFFF),

        // icons
        .icons = &blusys::icon_set_minimal(),
    };
    return tokens;
}

// ---- display-aware scaling ----

namespace {

// Scale a non-negative pixel token by ratio, floor at 1px.
// Preserves zero and negative values (e.g. radius_button = 999 is a
// sentinel meaning "pill" — scaling it preserves that intent).
inline int scale_px(int token, float ratio) noexcept
{
    if (token <= 0) {
        return token;
    }
    const int v = static_cast<int>(static_cast<float>(token) * ratio + 0.5f);
    return v < 1 ? 1 : v;
}

// Step fonts between the two available sizes.
// Below 0.75: use the smaller font.  At or above 0.75: keep the larger font.
// Non-Montserrat fonts (custom themes) are passed through unchanged.
inline const lv_font_t *scale_font(const lv_font_t *f, float ratio) noexcept
{
    const lv_font_t *sm = blusys::fonts::montserrat_14();
    const lv_font_t *lg = blusys::fonts::montserrat_20();
    if (f == sm || f == lg) {
        return (ratio < 0.75f) ? sm : lg;
    }
    return f;  // custom font — leave unchanged
}

// Auto-select the closest built-in base preset for a color display.
// The short-edge threshold (200 px) intentionally matches
// blusys::kTierBreakpointPx in display_constants.hpp — keep in sync.
// Not #included here: presets.cpp is UI layer, that header is platform layer.
const blusys::theme_tokens &auto_base(std::uint32_t w, std::uint32_t h,
                                       blusys::presets::panel_kind kind) noexcept
{
    if (kind == blusys::presets::panel_kind::mono_page) {
        return oled();
    }
    const std::uint32_t short_edge = (w < h) ? w : h;
    return (short_edge <= 200) ? operational_light() : expressive_dark();
}

}  // namespace

blusys::theme_tokens for_display(const blusys::theme_tokens &base,
                                  std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys::presets::panel_kind kind)
{
    // mono_page: return unchanged except for orientation — oled preset is pixel-perfect.
    if (kind == blusys::presets::panel_kind::mono_page) {
        blusys::theme_tokens t = base;
        t.orientation = blusys::orientation_for(actual_w, actual_h);
        return t;
    }

    // Guard against uninitialised design dimensions.
    // Still set orientation so callers get a meaningful value.
    if (base.design_w == 0 || base.design_h == 0) {
        blusys::theme_tokens t = base;
        t.orientation = blusys::orientation_for(actual_w, actual_h);
        return t;
    }

    // Normalise both sides to (short_edge, long_edge) so that a 320×240 preset
    // produces ratio 1.0 on a 240×320 portrait display instead of 0.75.
    const std::uint32_t short_actual  = (actual_w  < actual_h)  ? actual_w  : actual_h;
    const std::uint32_t long_actual   = (actual_w  < actual_h)  ? actual_h  : actual_w;
    const std::uint32_t short_design  = (base.design_w < base.design_h) ? base.design_w : base.design_h;
    const std::uint32_t long_design   = (base.design_w < base.design_h) ? base.design_h : base.design_w;
    const float sx    = static_cast<float>(short_actual) / static_cast<float>(short_design);
    const float sy    = static_cast<float>(long_actual)  / static_cast<float>(long_design);
    const float ratio = (sx < sy) ? sx : sy;

    blusys::theme_tokens t = base;  // copy — preserves colours and all non-pixel fields

    // --- pixel-unit tokens ---
    t.spacing_xs       = scale_px(base.spacing_xs,       ratio);
    t.spacing_sm       = scale_px(base.spacing_sm,       ratio);
    t.spacing_md       = scale_px(base.spacing_md,       ratio);
    t.spacing_lg       = scale_px(base.spacing_lg,       ratio);
    t.spacing_xl       = scale_px(base.spacing_xl,       ratio);
    t.spacing_2xl      = scale_px(base.spacing_2xl,      ratio);
    t.touch_target_min = scale_px(base.touch_target_min, ratio);
    t.radius_card      = scale_px(base.radius_card,      ratio);
    t.radius_button    = scale_px(base.radius_button,    ratio);

    t.shadow_card_width    = static_cast<std::uint8_t>(scale_px(
                                 static_cast<int>(base.shadow_card_width),    ratio));
    t.shadow_card_ofs_y    = static_cast<std::int16_t>(
                                 static_cast<float>(base.shadow_card_ofs_y)   * ratio);
    t.shadow_card_spread   = static_cast<std::uint8_t>(scale_px(
                                 static_cast<int>(base.shadow_card_spread),   ratio));
    t.shadow_overlay_width = static_cast<std::uint8_t>(scale_px(
                                 static_cast<int>(base.shadow_overlay_width), ratio));
    t.shadow_overlay_ofs_y = static_cast<std::int16_t>(
                                 static_cast<float>(base.shadow_overlay_ofs_y) * ratio);
    t.focus_ring_width     = scale_px(base.focus_ring_width, ratio);

    // --- font stepping ---
    t.font_body    = scale_font(base.font_body,    ratio);
    t.font_body_sm = scale_font(base.font_body_sm, ratio);
    t.font_title   = scale_font(base.font_title,   ratio);
    t.font_display = scale_font(base.font_display, ratio);
    t.font_label   = scale_font(base.font_label,   ratio);
    t.font_mono    = scale_font(base.font_mono,    ratio);

    // --- density_mode and orientation from scale ---
    // This is the sole production writer of both fields. Presets intentionally
    // do not set them (see Step 2a); they are authored here from actual dims.
    // compact:     scale < 0.6  (drives shell chrome collapse in shell_create)
    // normal:      0.6 ≤ scale ≤ 1.2
    // comfortable: scale > 1.2  (theme designed for a smaller display, shown on a larger one)
    if (ratio < 0.6f) {
        t.density_mode = blusys::density::compact;
    } else if (ratio > 1.2f) {
        t.density_mode = blusys::density::comfortable;
    } else {
        t.density_mode = blusys::density::normal;
    }
    t.orientation = blusys::orientation_for(actual_w, actual_h);

    // Update design dims so the returned token reflects the actual display.
    // Makes double-scaling idempotent (for_display applied twice gives the same result).
    t.design_w = actual_w;
    t.design_h = actual_h;

    return t;
}

blusys::theme_tokens for_display(std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys::presets::panel_kind kind)
{
    return for_display(auto_base(actual_w, actual_h, kind), actual_w, actual_h, kind);
}

}  // namespace blusys::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
