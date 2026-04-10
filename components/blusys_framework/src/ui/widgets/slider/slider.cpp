#include "blusys/framework/ui/widgets/slider/slider.hpp"

#include <cassert>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Camp 2 implementation: stock `lv_slider` wrapped with theme styling and
// a fixed-capacity slot pool for callback storage. The pattern mirrors
// `bu_button` and `bu_toggle` exactly — see `widget-author-guide.md` for
// the rules. The `LV_EVENT_VALUE_CHANGED` LVGL event is translated into
// a semantic `change_cb_t(int32_t, void*)` invocation by reading
// `lv_slider_get_value`.
//
// Programmatic value/range mutations via `slider_set_value` and
// `slider_set_range` deliberately bypass the callback: setters never
// trigger callbacks (rule from the author guide). The callback fires
// only on user-driven drag events.

#ifndef BLUSYS_UI_SLIDER_POOL_SIZE
#define BLUSYS_UI_SLIDER_POOL_SIZE 32
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.slider";

struct slider_slot {
    change_cb_t on_change;
    void       *user_data;
    bool        in_use;
};

slider_slot g_slider_slots[BLUSYS_UI_SLIDER_POOL_SIZE] = {};

slider_slot *acquire_slot()
{
    for (auto &s : g_slider_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_SLIDER_POOL_SIZE",
                BLUSYS_UI_SLIDER_POOL_SIZE);
    assert(false && "bu_slider slot pool exhausted");
    return nullptr;
}

void release_slot(slider_slot *slot)
{
    if (slot != nullptr) {
        slot->on_change = nullptr;
        slot->user_data = nullptr;
        slot->in_use    = false;
    }
}

void apply_theme(lv_obj_t *slider)
{
    const auto &t = theme();

    // Track (background).
    lv_obj_set_style_bg_color(slider, t.color_disabled, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, t.radius_button, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);

    // Indicator (filled portion).
    lv_obj_set_style_bg_color(slider, t.color_primary, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, t.radius_button, LV_PART_INDICATOR);

    // Knob.
    lv_obj_set_style_bg_color(slider, t.color_on_primary, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, t.spacing_sm, LV_PART_KNOB);

    // Disabled state.
    lv_obj_set_style_opa(slider, t.opa_disabled, LV_STATE_DISABLED);

    // Focus ring.
    lv_obj_set_style_outline_width(slider, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(slider, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(slider, 2, LV_STATE_FOCUSED);
}

void on_lvgl_value_changed(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<slider_slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr && slot->on_change != nullptr) {
        const int32_t new_value = lv_slider_get_value(obj);
        slot->on_change(new_value, slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<slider_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // namespace

lv_obj_t *slider_create(lv_obj_t *parent, const slider_config &config)
{
    slider_slot *slot = nullptr;
    if (config.on_change != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
    }

    lv_obj_t *slider = lv_slider_create(parent);
    apply_theme(slider);

    lv_slider_set_range(slider, config.min, config.max);
    lv_slider_set_value(slider, config.initial, LV_ANIM_OFF);

    if (slot != nullptr) {
        slot->on_change = config.on_change;
        slot->user_data = config.user_data;
        lv_obj_set_user_data(slider, slot);
        lv_obj_add_event_cb(slider, on_lvgl_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_add_event_cb(slider, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);
    }

    if (config.disabled) {
        lv_obj_add_state(slider, LV_STATE_DISABLED);
    }

    return slider;
}

void slider_set_value(lv_obj_t *slider, int32_t value)
{
    if (slider == nullptr) {
        return;
    }
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
}

int32_t slider_get_value(lv_obj_t *slider)
{
    if (slider == nullptr) {
        return 0;
    }
    return lv_slider_get_value(slider);
}

void slider_set_range(lv_obj_t *slider, int32_t min, int32_t max)
{
    if (slider == nullptr) {
        return;
    }
    lv_slider_set_range(slider, min, max);
}

void slider_set_disabled(lv_obj_t *slider, bool disabled)
{
    if (slider == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(slider, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(slider, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui
