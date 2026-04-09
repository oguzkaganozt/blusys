#pragma once

#include "lvgl.h"

namespace blusys::ui {

struct col_config {
    int gap = 0;
    int padding = 0;
};

lv_obj_t *col_create(lv_obj_t *parent, const col_config &config);

}  // namespace blusys::ui
