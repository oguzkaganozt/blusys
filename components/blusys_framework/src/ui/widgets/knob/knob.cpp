#include "blusys/framework/ui/widgets/knob/knob.hpp"

#include <cassert>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Interactive rotary arc control wrapping `lv_arc`.
//
// Follows the slider pattern exactly: `LV_EVENT_VALUE_CHANGED` maps to
// `change_cb_t(value, user_data)`. Setter discipline: `knob_set_value`
// does not fire the callback.

#ifndef BLUSYS_UI_KNOB_POOL_SIZE
#define BLUSYS_UI_KNOB_POOL_SIZE 16
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.knob";

struct knob_slot {
    change_cb_t on_change;
    void       *user_data;
    bool        in_use;
};

knob_slot g_knob_slots[BLUSYS_UI_KNOB_POOL_SIZE] = {};

knob_slot *acquire_slot()
{
    for (auto &s : g_knob_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_KNOB_POOL_SIZE",
                BLUSYS_UI_KNOB_POOL_SIZE);
    assert(false && "bu_knob slot pool exhausted");
    return nullptr;
}

void release_slot(knob_slot *slot)
{
    if (slot != nullptr) {
        slot->on_change = nullptr;
        slot->user_data = nullptr;
        slot->in_use    = false;
    }
}

void apply_theme(lv_obj_t *arc)
{
    const auto &t = theme();

    // Background arc (track).
    lv_obj_set_style_arc_color(arc, t.color_disabled, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);

    // Indicator arc (filled portion).
    lv_obj_set_style_arc_color(arc, t.color_primary, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);

    // Knob handle.
    lv_obj_set_style_bg_color(arc, t.color_on_primary, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, t.spacing_xs, LV_PART_KNOB);

    // Focus ring.
    lv_obj_set_style_outline_width(arc, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(arc, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(arc, 2, LV_STATE_FOCUSED);

    // Disabled state.
    lv_obj_set_style_opa(arc, t.opa_disabled, LV_STATE_DISABLED);
}

void on_lvgl_value_changed(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<knob_slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr && slot->on_change != nullptr) {
        int32_t value = lv_arc_get_value(obj);
        slot->on_change(value, slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<knob_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // namespace

lv_obj_t *knob_create(lv_obj_t *parent, const knob_config &config)
{
    knob_slot *slot = nullptr;
    if (config.on_change != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
        slot->on_change = config.on_change;
        slot->user_data = config.user_data;
    }

    lv_obj_t *arc = lv_arc_create(parent);
    apply_theme(arc);

    lv_arc_set_rotation(arc, static_cast<uint16_t>(config.start_angle));
    lv_arc_set_bg_angles(arc, 0,
                         static_cast<uint16_t>(config.end_angle - config.start_angle));
    lv_arc_set_range(arc, config.min, config.max);
    lv_arc_set_value(arc, config.initial);

    if (slot != nullptr) {
        lv_obj_set_user_data(arc, slot);
        lv_obj_add_event_cb(arc, on_lvgl_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_add_event_cb(arc, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);
    }

    if (config.disabled) {
        lv_obj_add_state(arc, LV_STATE_DISABLED);
    }

    return arc;
}

void knob_set_value(lv_obj_t *knob, int32_t value)
{
    if (knob == nullptr) {
        return;
    }
    lv_arc_set_value(knob, value);
}

int32_t knob_get_value(lv_obj_t *knob)
{
    if (knob == nullptr) {
        return 0;
    }
    return lv_arc_get_value(knob);
}

void knob_set_range(lv_obj_t *knob, int32_t min, int32_t max)
{
    if (knob == nullptr) {
        return;
    }
    lv_arc_set_range(knob, min, max);
}

void knob_set_disabled(lv_obj_t *knob, bool disabled)
{
    if (knob == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(knob, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(knob, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui
