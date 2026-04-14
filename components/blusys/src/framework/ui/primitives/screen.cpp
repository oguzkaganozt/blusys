#include "blusys/framework/ui/primitives/screen.hpp"

#include "blusys/framework/ui/style/theme.hpp"

namespace blusys {

lv_obj_t *screen_create(const screen_config &config)
{
    lv_obj_t *screen = lv_obj_create(nullptr);
    const auto &t = theme();

    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    if (config.scrollable) {
        lv_obj_add_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_set_style_bg_color(screen, t.color_background, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_radius(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));

    return screen;
}

}  // namespace blusys
