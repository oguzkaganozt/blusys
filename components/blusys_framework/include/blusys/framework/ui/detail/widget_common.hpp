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

// Shared LV_EVENT_DELETE callback for Camp-2 pool widgets. Recovers the slot
// from lv_obj_get_user_data() and zero-inits it so the pool slot is free again.
template <typename Slot>
void release_slot_on_delete(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<Slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr) {
        *slot = {};
    }
}

}  // namespace blusys::ui::detail
