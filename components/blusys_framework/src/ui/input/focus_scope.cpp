#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/input/focus_scope.hpp"
#include "blusys/framework/ui/input/encoder.hpp"
#include "blusys/log.h"

static const char *TAG = "focus_scope";

namespace blusys::ui {

bool focus_scope_manager::push_scope(lv_obj_t *container)
{
    if (container == nullptr) {
        return false;
    }
    if (stack_.size() >= kMaxScopes) {
        BLUSYS_LOGW(TAG, "focus scope stack full (max %zu)", kMaxScopes);
        return false;
    }

    lv_group_t *group = create_encoder_group();
    if (group == nullptr) {
        BLUSYS_LOGW(TAG, "failed to create focus group");
        return false;
    }

    auto_focus_screen(container, group);
    bind_group_to_encoders(group);

    (void)stack_.push_back({.container = container, .group = group});
    return true;
}

void focus_scope_manager::pop_scope()
{
    if (stack_.empty()) {
        return;
    }

    // Destroy the top scope's group.
    auto &top = stack_[stack_.size() - 1];
    if (top.group != nullptr) {
        lv_group_delete(top.group);
    }
    stack_.pop_back();

    // Restore the previous scope's group to encoder indevs.
    if (!stack_.empty()) {
        bind_group_to_encoders(stack_[stack_.size() - 1].group);
    }
}

void focus_scope_manager::refresh_current()
{
    if (stack_.empty()) {
        return;
    }
    auto &top = stack_[stack_.size() - 1];
    if (top.group == nullptr || top.container == nullptr) {
        return;
    }
    lv_group_remove_all_objs(top.group);
    auto_focus_screen(top.container, top.group);
}

lv_group_t *focus_scope_manager::current_group() const
{
    if (stack_.empty()) {
        return nullptr;
    }
    return stack_[stack_.size() - 1].group;
}

void focus_scope_manager::reset()
{
    while (!stack_.empty()) {
        auto &top = stack_[stack_.size() - 1];
        if (top.group != nullptr) {
            lv_group_delete(top.group);
        }
        stack_.pop_back();
    }
}

void focus_scope_manager::bind_group_to_encoders(lv_group_t *group)
{
    lv_indev_t *indev = lv_indev_get_next(nullptr);
    while (indev != nullptr) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
            lv_indev_set_group(indev, group);
        }
        indev = lv_indev_get_next(indev);
    }
}

}  // namespace blusys::ui

#endif  // BLUSYS_FRAMEWORK_HAS_UI
