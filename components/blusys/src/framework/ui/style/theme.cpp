#include "blusys/framework/ui/style/theme.hpp"

namespace blusys {
namespace {

theme_tokens make_default_theme()
{
    return {
        // core palette
        .color_primary    = lv_color_hex(0x275DFF),
        .color_surface    = lv_color_hex(0x12151F),
        .color_on_primary = lv_color_hex(0xF6F8FF),
        .color_accent     = lv_color_hex(0x59D0C2),
        .color_disabled   = lv_color_hex(0x5E657A),

        // semantic status
        .color_success    = lv_color_hex(0x22C55E),
        .color_warning    = lv_color_hex(0xF59E0B),
        .color_error      = lv_color_hex(0xEF4444),
        .color_info       = lv_color_hex(0x3B82F6),
        .color_on_success = lv_color_hex(0xFFFFFF),
        .color_on_warning = lv_color_hex(0x1A1A1A),
        .color_on_error   = lv_color_hex(0xFFFFFF),

        // surface and outline
        .color_background       = lv_color_hex(0x0D1017),
        .color_on_surface       = lv_color_hex(0xE0E4EC),
        .color_surface_elevated = lv_color_hex(0x181C27),
        .color_outline          = lv_color_hex(0x3A3F4B),
        .color_outline_variant  = lv_color_hex(0x2A2E38),

        // density
        .density_mode = density::normal,

        .feedback_voice = theme_feedback_voice::expressive,

        // spacing
        .spacing_xs  = 4,
        .spacing_sm  = 8,
        .spacing_md  = 12,
        .spacing_lg  = 16,
        .spacing_xl  = 24,
        .spacing_2xl = 32,

        .touch_target_min = 40,

        // radii
        .radius_card   = 12,
        .radius_button = 999,

        // typography
        .font_body    = LV_FONT_DEFAULT,
        .font_body_sm = LV_FONT_DEFAULT,
        .font_title   = LV_FONT_DEFAULT,
        .font_display = LV_FONT_DEFAULT,
        .font_label   = LV_FONT_DEFAULT,
        .font_mono    = LV_FONT_DEFAULT,

        .text_letter_space_body = 0,

        // motion
        .anim_duration_fast   = 100,
        .anim_duration_normal = 200,
        .anim_duration_slow   = 350,
        .anim_path_standard   = lv_anim_path_ease_in_out,
        .anim_path_enter      = lv_anim_path_ease_out,
        .anim_path_exit       = lv_anim_path_ease_in,
        .anim_enabled         = true,

        // depth / elevation
        .shadow_card_width  = 8,
        .shadow_card_ofs_y  = 4,
        .shadow_card_spread = 2,
        .shadow_color       = lv_color_hex(0x000000),

        .shadow_overlay_width = 16,
        .shadow_overlay_ofs_y = 8,

        // interaction state
        .opa_pressed       = LV_OPA_80,
        .opa_focused       = LV_OPA_40,
        .opa_disabled      = LV_OPA_30,
        .opa_backdrop         = LV_OPA_70,
        .opa_surface_subtle   = LV_OPA_20,
        .opa_shadow_soft      = LV_OPA_40,
        .focus_ring_width  = 2,
        .color_focus_ring  = lv_color_hex(0x275DFF),

        // icons
        .icons = nullptr,
    };
}

theme_tokens s_theme = make_default_theme();

}  // namespace

void set_theme(const theme_tokens &tokens)
{
    s_theme = tokens;
}

const theme_tokens &theme()
{
    return s_theme;
}

}  // namespace blusys
