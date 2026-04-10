#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/theme_presets.hpp"

#include "lvgl.h"

namespace blusys::app::presets {

const blusys::ui::theme_tokens &dark()
{
    static const blusys::ui::theme_tokens tokens{
        .color_primary    = lv_color_hex(0x2A62FF),
        .color_surface    = lv_color_hex(0x11141D),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        .color_accent     = lv_color_hex(0x69D6C8),
        .color_disabled   = lv_color_hex(0x667089),
        .spacing_sm       = 8,
        .spacing_md       = 12,
        .spacing_lg       = 20,
        .radius_card      = 14,
        .radius_button    = 999,
        .font_body        = LV_FONT_DEFAULT,
        .font_title       = LV_FONT_DEFAULT,
        .font_mono        = LV_FONT_DEFAULT,
    };
    return tokens;
}

const blusys::ui::theme_tokens &oled()
{
    static const blusys::ui::theme_tokens tokens{
        .color_primary    = lv_color_hex(0xFFFFFF),
        .color_surface    = lv_color_hex(0x000000),
        .color_on_primary = lv_color_hex(0x000000),
        .color_accent     = lv_color_hex(0xFFFFFF),
        .color_disabled   = lv_color_hex(0x555555),
        .spacing_sm       = 2,
        .spacing_md       = 4,
        .spacing_lg       = 8,
        .radius_card      = 0,
        .radius_button    = 0,
        .font_body        = LV_FONT_DEFAULT,
        .font_title       = LV_FONT_DEFAULT,
        .font_mono        = LV_FONT_DEFAULT,
    };
    return tokens;
}

const blusys::ui::theme_tokens &light()
{
    static const blusys::ui::theme_tokens tokens{
        .color_primary    = lv_color_hex(0x2A62FF),
        .color_surface    = lv_color_hex(0xF0F2F5),
        .color_on_primary = lv_color_hex(0xFFFFFF),
        .color_accent     = lv_color_hex(0x00897B),
        .color_disabled   = lv_color_hex(0xBDBDBD),
        .spacing_sm       = 8,
        .spacing_md       = 12,
        .spacing_lg       = 20,
        .radius_card      = 14,
        .radius_button    = 999,
        .font_body        = LV_FONT_DEFAULT,
        .font_title       = LV_FONT_DEFAULT,
        .font_mono        = LV_FONT_DEFAULT,
    };
    return tokens;
}

}  // namespace blusys::app::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
