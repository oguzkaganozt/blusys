#pragma once

#include "lvgl.h"

namespace blusys::ui {

struct label_config {
    const char      *text = "";
    const lv_font_t *font = nullptr;
};

lv_obj_t *label_create(lv_obj_t *parent, const label_config &config);

}  // namespace blusys::ui
