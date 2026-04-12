#include "blusys/framework/ui/widgets/knob/knob.hpp"

#include "blusys/framework/ui/detail/widget_common.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Interactive rotary arc control wrapping `lv_arc`.
//
// Camp 3 pattern: per-widget state is allocated with LVGL's allocator when a
// callback is registered, and freed on LV_EVENT_DELETE — no global slot pool.

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.knob";

struct knob_data {
    change_cb_t on_change = nullptr;
    void       *user_data = nullptr;
};

void apply_theme(lv_obj_t *arc)
{
    const auto &t = theme();

    lv_obj_set_style_arc_color(arc, t.color_disabled, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_set_style_arc_color(arc, t.color_primary, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_obj_set_style_bg_color(arc, t.color_on_primary, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, t.spacing_xs, LV_PART_KNOB);

    lv_obj_set_style_outline_width(arc, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(arc, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(arc, 2, LV_STATE_FOCUSED);

    lv_obj_set_style_opa(arc, t.opa_disabled, LV_STATE_DISABLED);
}

void on_lvgl_value_changed(lv_event_t *e)
{
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *d   = static_cast<knob_data *>(lv_obj_get_user_data(obj));
    if (d != nullptr && d->on_change != nullptr) {
        const int32_t value = lv_arc_get_value(obj);
        d->on_change(value, d->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *d   = static_cast<knob_data *>(lv_obj_get_user_data(obj));
    if (d != nullptr) {
        lv_free(d);
        lv_obj_set_user_data(obj, nullptr);
    }
}

}  // namespace

lv_obj_t *knob_create(lv_obj_t *parent, const knob_config &config)
{
    knob_data *data = nullptr;
    if (config.on_change != nullptr) {
        data = static_cast<knob_data *>(lv_malloc(sizeof(knob_data)));
        if (data == nullptr) {
            BLUSYS_LOGE(kTag, "lv_malloc failed for bu_knob user data");
            return nullptr;
        }
        data->on_change = config.on_change;
        data->user_data = config.user_data;
    }

    lv_obj_t *arc = lv_arc_create(parent);
    if (arc == nullptr) {
        if (data != nullptr) {
            lv_free(data);
        }
        return nullptr;
    }

    apply_theme(arc);

    lv_arc_set_rotation(arc, static_cast<uint16_t>(config.start_angle));
    lv_arc_set_bg_angles(arc, 0,
                         static_cast<uint16_t>(config.end_angle - config.start_angle));
    lv_arc_set_range(arc, config.min, config.max);
    lv_arc_set_value(arc, config.initial);

    if (data != nullptr) {
        lv_obj_set_user_data(arc, data);
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
    detail::set_widget_disabled(knob, disabled);
}

}  // namespace blusys::ui
