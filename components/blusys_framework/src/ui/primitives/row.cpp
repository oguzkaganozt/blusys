#include "blusys/framework/ui/primitives/row.hpp"

#include "blusys/framework/ui/theme.hpp"

namespace blusys::ui {

lv_obj_t *row_create(lv_obj_t *parent, const row_config &config)
{
    lv_obj_t *row = lv_obj_create(parent);
    const auto &t = theme();

    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, config.main_place, config.cross_place, config.track_place);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, config.padding > 0 ? config.padding : t.spacing_sm, 0);
    lv_obj_set_style_pad_column(row, config.gap, 0);
    lv_obj_set_style_pad_row(row, config.gap, 0);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    return row;
}

}  // namespace blusys::ui
