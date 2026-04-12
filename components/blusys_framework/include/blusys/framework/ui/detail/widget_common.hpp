#pragma once

#include "lvgl.h"

namespace blusys::ui::detail {

inline void set_widget_disabled(lv_obj_t *w, bool disabled)
{
    if (w == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(w, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(w, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui::detail
