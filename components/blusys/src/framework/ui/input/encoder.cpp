#include "blusys/framework/ui/input/encoder.hpp"

#include <cstdint>

#include "blusys/hal/log.h"

namespace blusys {
namespace {

constexpr const char *kTag = "ui.input.encoder";

void walk_and_add(lv_obj_t *obj, lv_group_t *group, bool *focused_first)
{
    if (obj == nullptr) {
        return;
    }
    // Prune hidden subtrees: a hidden parent is not visible on screen,
    // so none of its descendants should participate in encoder focus.
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
        lv_group_add_obj(group, obj);
        if (!*focused_first) {
            lv_group_focus_obj(obj);
            *focused_first = true;
        }
    }
    const uint32_t count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < count; ++i) {
        walk_and_add(lv_obj_get_child(obj, i), group, focused_first);
    }
}

}  // namespace

lv_group_t *create_encoder_group()
{
    lv_group_t *group = lv_group_create();
    if (group == nullptr) {
        BLUSYS_LOGE(kTag, "lv_group_create returned nullptr");
    }
    return group;
}

void auto_focus_screen(lv_obj_t *screen, lv_group_t *group)
{
    if (screen == nullptr || group == nullptr) {
        return;
    }
    bool focused_first = false;
    walk_and_add(screen, group, &focused_first);
}

}  // namespace blusys
