#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/theme_presets.hpp"

#include "blusys/framework/ui/fonts.hpp"
#include "blusys/framework/ui/icons/icon_set_minimal.hpp"

#include "lvgl.h"

namespace blusys::app::presets {

const blusys::ui::theme_tokens &expressive_dark()
{
    static const blusys::ui::theme_tokens tokens{
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

        // density — comfortable for consumer / controller
        .density_mode = blusys::ui::density::comfortable,

        .feedback_voice = blusys::ui::theme_feedback_voice::expressive,

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
        .font_body    = blusys::ui::fonts::montserrat_14(),
        .font_body_sm = blusys::ui::fonts::montserrat_14(),
        .font_title   = blusys::ui::fonts::montserrat_20(),
        .font_display = blusys::ui::fonts::montserrat_20(),
        .font_label   = blusys::ui::fonts::montserrat_14(),
        .font_mono    = blusys::ui::fonts::montserrat_14(),

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
        .icons = &blusys::ui::icon_set_minimal(),
    };
    return tokens;
}

const blusys::ui::theme_tokens &operational_light()
{
    static const blusys::ui::theme_tokens tokens{
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

        // density — compact for industrial
        .density_mode = blusys::ui::density::compact,

        .feedback_voice = blusys::ui::theme_feedback_voice::operational,

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
        .font_body    = blusys::ui::fonts::montserrat_14(),
        .font_body_sm = blusys::ui::fonts::montserrat_14(),
        .font_title   = blusys::ui::fonts::montserrat_14(),
        .font_display = blusys::ui::fonts::montserrat_14(),
        .font_label   = blusys::ui::fonts::montserrat_14(),
        .font_mono    = blusys::ui::fonts::montserrat_14(),

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
        .icons = &blusys::ui::icon_set_minimal(),
    };
    return tokens;
}

const blusys::ui::theme_tokens &oled()
{
    static const blusys::ui::theme_tokens tokens{
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

        // density — ultra compact
        .density_mode = blusys::ui::density::compact,

        .feedback_voice = blusys::ui::theme_feedback_voice::operational,

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
        .icons = &blusys::ui::icon_set_minimal(),
    };
    return tokens;
}

}  // namespace blusys::app::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
