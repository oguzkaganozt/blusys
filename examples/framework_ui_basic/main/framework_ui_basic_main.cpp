#include "blusys/framework/ui/widgets.hpp"

#include "blusys/log.h"

extern "C" void app_main(void)
{
    lv_init();

    blusys::ui::set_theme({
        .color_primary = lv_color_hex(0x2A62FF),
        .color_surface = lv_color_hex(0x11141D),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        .color_accent = lv_color_hex(0x69D6C8),
        .color_disabled = lv_color_hex(0x667089),
        .spacing_sm = 8,
        .spacing_md = 12,
        .spacing_lg = 20,
        .radius_card = 14,
        .radius_button = 999,
        .font_body = LV_FONT_DEFAULT,
        .font_title = LV_FONT_DEFAULT,
        .font_mono = LV_FONT_DEFAULT,
    });

    lv_obj_t *screen = blusys::ui::screen_create({});
    lv_obj_t *col = blusys::ui::col_create(screen, {
        .gap = blusys::ui::theme().spacing_md,
        .padding = blusys::ui::theme().spacing_lg,
    });
    blusys::ui::label_create(col, {
        .text = "Framework UI",
        .font = blusys::ui::theme().font_title,
    });
    blusys::ui::divider_create(col, {});
    lv_obj_t *row = blusys::ui::row_create(col, {
        .gap = blusys::ui::theme().spacing_sm,
        .padding = 0,
    });
    blusys::ui::label_create(row, {
        .text = "Theme and primitives compile.",
        .font = blusys::ui::theme().font_body,
    });

    BLUSYS_LOGI("framework_ui_basic", "screen=%p row=%p", static_cast<void *>(screen), static_cast<void *>(row));
}
