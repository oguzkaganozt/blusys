#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/hal/log.h"
#include "lvgl.h"

#include <cassert>

namespace blusys::detail {

namespace {

constexpr const char *kTag = "blusys_app";

action_dispatch_slot g_action_slots[BLUSYS_APP_ACTION_SLOT_POOL_SIZE] = {};

}  // namespace

action_dispatch_slot *acquire_action_slot()
{
    for (auto &slot : g_action_slots) {
        if (!slot.in_use) {
            slot.in_use = true;
            return &slot;
        }
    }

    BLUSYS_LOGE(kTag, "action dispatch slot pool exhausted (size=%d)",
                BLUSYS_APP_ACTION_SLOT_POOL_SIZE);
    assert(false);
    return nullptr;
}

void release_action_slot(action_dispatch_slot *slot)
{
    if (slot != nullptr) {
        slot->in_use = false;
        slot->ctx = nullptr;
        slot->dispatch_fn = nullptr;
    }
}

void attach_slot_cleanup(lv_obj_t *widget, action_dispatch_slot *slot)
{
    // Register a delete handler that releases the action slot when
    // LVGL destroys the widget (e.g. screen change, explicit delete).
    // The slot pointer is passed through the event's user_data.
    lv_obj_add_event_cb(
        widget,
        +[](lv_event_t *event) {
            auto *s = static_cast<action_dispatch_slot *>(lv_event_get_user_data(event));
            release_action_slot(s);
        },
        LV_EVENT_DELETE,
        slot);
}

}  // namespace blusys::detail

#endif  // BLUSYS_FRAMEWORK_HAS_UI
