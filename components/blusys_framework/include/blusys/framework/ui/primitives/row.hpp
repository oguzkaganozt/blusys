#pragma once

#include "lvgl.h"

namespace blusys::ui {

struct row_config {
    int gap = 0;
    int padding = 0;
};

lv_obj_t *row_create(lv_obj_t *parent, const row_config &config);

}  // namespace blusys::ui
