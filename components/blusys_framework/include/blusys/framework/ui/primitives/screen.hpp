#pragma once

#include "lvgl.h"

namespace blusys::ui {

struct screen_config {
    bool scrollable = false;
};

lv_obj_t *screen_create(const screen_config &config);

}  // namespace blusys::ui
