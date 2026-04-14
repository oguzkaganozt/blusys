#include "blusys/framework/ui/primitives/divider.hpp"

#include "blusys/framework/ui/style/theme.hpp"

namespace blusys {

lv_obj_t *divider_create(lv_obj_t *parent, const divider_config &config)
{
    lv_obj_t *divider = lv_obj_create(parent);
    const auto &t = theme();

    lv_obj_remove_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(divider, t.color_outline_variant, 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);
    lv_obj_set_style_pad_all(divider, 0, 0);
    lv_obj_set_size(divider, LV_PCT(100), config.thickness);

    return divider;
}

}  // namespace blusys
