#include "blusys/framework/ui/primitives/col.hpp"

#include "blusys/framework/ui/theme.hpp"

namespace blusys::ui {

lv_obj_t *col_create(lv_obj_t *parent, const col_config &config)
{
    lv_obj_t *col = lv_obj_create(parent);
    const auto &t = theme();

    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, config.padding > 0 ? config.padding : t.spacing_sm, 0);
    lv_obj_set_style_pad_column(col, config.gap, 0);
    lv_obj_set_style_pad_row(col, config.gap, 0);
    lv_obj_set_width(col, LV_PCT(100));
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    return col;
}

}  // namespace blusys::ui
