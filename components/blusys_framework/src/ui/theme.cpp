#include "blusys/framework/ui/theme.hpp"

namespace blusys::ui {
namespace {

theme_tokens make_default_theme()
{
    return {
        .color_primary = lv_color_hex(0x275DFF),
        .color_surface = lv_color_hex(0x12151F),
        .color_on_primary = lv_color_hex(0xF6F8FF),
        .color_accent = lv_color_hex(0x59D0C2),
        .color_disabled = lv_color_hex(0x5E657A),
        .spacing_sm = 8,
        .spacing_md = 12,
        .spacing_lg = 16,
        .radius_card = 12,
        .radius_button = 999,
        .font_body = LV_FONT_DEFAULT,
        .font_title = LV_FONT_DEFAULT,
        .font_mono = LV_FONT_DEFAULT,
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

}  // namespace blusys::ui
