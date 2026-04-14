#pragma once

#include "lvgl.h"

namespace blusys {

struct col_config {
    int gap = 0;
    int padding = 0;
    /// LVGL `lv_obj_set_flex_align` main / cross / track (defaults match prior behavior).
    lv_flex_align_t main_place  = LV_FLEX_ALIGN_START;
    lv_flex_align_t cross_place = LV_FLEX_ALIGN_START;
    lv_flex_align_t track_place = LV_FLEX_ALIGN_START;
};

lv_obj_t *col_create(lv_obj_t *parent, const col_config &config);

}  // namespace blusys
