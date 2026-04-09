#pragma once

#include "lvgl.h"

namespace blusys::ui {

struct divider_config {
    int thickness = 1;
};

lv_obj_t *divider_create(lv_obj_t *parent, const divider_config &config);

}  // namespace blusys::ui
