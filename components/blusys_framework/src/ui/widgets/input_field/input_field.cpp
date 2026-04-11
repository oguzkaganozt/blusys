#include "blusys/framework/ui/widgets/input_field/input_field.hpp"

#include <cassert>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Themed text input wrapping `lv_textarea`.
//
// `LV_EVENT_READY` (fired on enter/confirm) maps to
// `text_cb_t(text, user_data)`. Setters do not fire callbacks.

#ifndef BLUSYS_UI_INPUT_FIELD_POOL_SIZE
#define BLUSYS_UI_INPUT_FIELD_POOL_SIZE 16
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.input_field";

struct input_field_slot {
    text_cb_t  on_submit;
    void      *user_data;
    bool       in_use;
};

input_field_slot g_input_field_slots[BLUSYS_UI_INPUT_FIELD_POOL_SIZE] = {};

input_field_slot *acquire_slot()
{
    for (auto &s : g_input_field_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_INPUT_FIELD_POOL_SIZE",
                BLUSYS_UI_INPUT_FIELD_POOL_SIZE);
    assert(false && "bu_input_field slot pool exhausted");
    return nullptr;
}

void release_slot(input_field_slot *slot)
{
    if (slot != nullptr) {
        slot->on_submit = nullptr;
        slot->user_data = nullptr;
        slot->in_use    = false;
    }
}

void apply_theme(lv_obj_t *ta)
{
    const auto &t = theme();

    // Background.
    lv_obj_set_style_bg_color(ta, t.color_surface_elevated, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN);

    // Border.
    lv_obj_set_style_border_color(ta, t.color_outline, LV_PART_MAIN);
    lv_obj_set_style_border_width(ta, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(ta, t.radius_button, LV_PART_MAIN);

    // Text.
    lv_obj_set_style_text_color(ta, t.color_on_surface, LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, t.font_body, LV_PART_MAIN);

    // Padding.
    lv_obj_set_style_pad_left(ta, t.spacing_sm, LV_PART_MAIN);
    lv_obj_set_style_pad_right(ta, t.spacing_sm, LV_PART_MAIN);
    lv_obj_set_style_pad_top(ta, t.spacing_sm, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(ta, t.spacing_sm, LV_PART_MAIN);

    // Min height.
    lv_obj_set_style_min_height(ta, t.touch_target_min, LV_PART_MAIN);

    // Focus state.
    lv_obj_set_style_border_color(ta, t.color_primary, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(ta, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(ta, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(ta, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(ta, 2, LV_STATE_FOCUSED);

    // Cursor.
    lv_obj_set_style_bg_color(ta, t.color_primary, LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_CURSOR);

    // Placeholder styling.
    lv_obj_set_style_text_color(ta, t.color_on_surface, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_opa(ta, t.opa_disabled, LV_PART_TEXTAREA_PLACEHOLDER);

    // Disabled state.
    lv_obj_set_style_opa(ta, t.opa_disabled, LV_STATE_DISABLED);
}

void on_lvgl_ready(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<input_field_slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr && slot->on_submit != nullptr) {
        const char *text = lv_textarea_get_text(obj);
        slot->on_submit(text, slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<input_field_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // namespace

lv_obj_t *input_field_create(lv_obj_t *parent, const input_field_config &config)
{
    input_field_slot *slot = nullptr;
    if (config.on_submit != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
        slot->on_submit = config.on_submit;
        slot->user_data = config.user_data;
    }

    lv_obj_t *ta = lv_textarea_create(parent);
    apply_theme(ta);

    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, static_cast<uint32_t>(config.max_length));

    if (config.placeholder != nullptr) {
        lv_textarea_set_placeholder_text(ta, config.placeholder);
    }

    if (config.initial != nullptr) {
        lv_textarea_set_text(ta, config.initial);
    }

    if (config.password) {
        lv_textarea_set_password_mode(ta, true);
    }

    lv_obj_set_width(ta, LV_PCT(100));

    if (slot != nullptr) {
        lv_obj_set_user_data(ta, slot);
        lv_obj_add_event_cb(ta, on_lvgl_ready, LV_EVENT_READY, nullptr);
        lv_obj_add_event_cb(ta, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);
    }

    if (config.disabled) {
        lv_obj_add_state(ta, LV_STATE_DISABLED);
    }

    return ta;
}

void input_field_set_text(lv_obj_t *input, const char *text)
{
    if (input == nullptr) {
        return;
    }
    lv_textarea_set_text(input, text != nullptr ? text : "");
}

const char *input_field_get_text(lv_obj_t *input)
{
    if (input == nullptr) {
        return "";
    }
    return lv_textarea_get_text(input);
}

void input_field_set_placeholder(lv_obj_t *input, const char *text)
{
    if (input == nullptr) {
        return;
    }
    lv_textarea_set_placeholder_text(input, text != nullptr ? text : "");
}

void input_field_set_disabled(lv_obj_t *input, bool disabled)
{
    if (input == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(input, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(input, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui
