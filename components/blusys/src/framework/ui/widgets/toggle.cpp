#include "blusys/framework/ui/widgets/toggle.hpp"

#include "../fixed_slot_pool.hpp"
#include "../widget_common.hpp"
#include "blusys/framework/ui/style/theme.hpp"

// Camp 2 implementation: stock `lv_switch` wrapped with theme styling and
// a fixed-capacity slot pool for callback storage. The pattern mirrors
// `bu_button` exactly — see `widget-author-guide.md` for the rules. The
// `LV_EVENT_VALUE_CHANGED` LVGL event is translated into a semantic
// `toggle_cb_t(bool, void*)` invocation.
//
// Programmatic state changes via `toggle_set_state` deliberately bypass
// the callback: setters never trigger callbacks (rule from the author
// guide). The callback fires only on user-driven flips.

#ifndef BLUSYS_UI_TOGGLE_POOL_SIZE
#define BLUSYS_UI_TOGGLE_POOL_SIZE 32
#endif

namespace blusys {
namespace {

struct toggle_slot {
    toggle_cb_t on_change;
    void       *user_data;
    bool        in_use;
};

detail::slot_pool<toggle_slot, BLUSYS_UI_TOGGLE_POOL_SIZE> g_toggle_pool{
    "ui.toggle", "BLUSYS_UI_TOGGLE_POOL_SIZE"};

void apply_theme(lv_obj_t *toggle)
{
    const auto &t = theme();

    // Track (off state).
    lv_obj_set_style_bg_color(toggle, t.color_disabled, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(toggle, t.radius_button, LV_PART_MAIN);
    lv_obj_set_style_border_width(toggle, 0, LV_PART_MAIN);

    // Indicator (on state — track when checked).
    lv_obj_set_style_bg_color(toggle, t.color_primary, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_INDICATOR);

    // Knob.
    lv_obj_set_style_bg_color(toggle, t.color_on_primary, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_KNOB);

    // Disabled state.
    lv_obj_set_style_opa(toggle, t.opa_disabled, LV_STATE_DISABLED);

    // Focus ring.
    lv_obj_set_style_outline_width(toggle, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(toggle, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(toggle, 2, LV_STATE_FOCUSED);
}

void on_lvgl_value_changed(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<toggle_slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr && slot->on_change != nullptr) {
        const bool new_state = lv_obj_has_state(obj, LV_STATE_CHECKED);
        slot->on_change(new_state, slot->user_data);
    }
}

}  // namespace

lv_obj_t *toggle_create(lv_obj_t *parent, const toggle_config &config)
{
    toggle_slot *slot = nullptr;
    if (config.on_change != nullptr) {
        slot = g_toggle_pool.acquire();
        if (slot == nullptr) {
            return nullptr;
        }
    }

    lv_obj_t *toggle = lv_switch_create(parent);
    apply_theme(toggle);

    if (config.initial) {
        lv_obj_add_state(toggle, LV_STATE_CHECKED);
    }

    if (slot != nullptr) {
        slot->on_change = config.on_change;
        slot->user_data = config.user_data;
        lv_obj_set_user_data(toggle, slot);
        lv_obj_add_event_cb(toggle, on_lvgl_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_add_event_cb(toggle, detail::release_slot_on_delete<toggle_slot>,
                            LV_EVENT_DELETE, nullptr);
    }

    detail::set_widget_disabled(toggle, config.disabled);

    return toggle;
}

void toggle_set_state(lv_obj_t *toggle, bool on)
{
    if (toggle == nullptr) {
        return;
    }
    if (on) {
        lv_obj_add_state(toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(toggle, LV_STATE_CHECKED);
    }
}

bool toggle_get_state(lv_obj_t *toggle)
{
    if (toggle == nullptr) {
        return false;
    }
    return lv_obj_has_state(toggle, LV_STATE_CHECKED);
}

void toggle_set_disabled(lv_obj_t *toggle, bool disabled)
{
    detail::set_widget_disabled(toggle, disabled);
}

}  // namespace blusys
