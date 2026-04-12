#include "blusys/framework/ui/widgets/button/button.hpp"

#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"
#include "blusys/framework/ui/detail/widget_common.hpp"
#include "blusys/framework/ui/theme.hpp"

// Camp 2 implementation: stock `lv_button` wrapped with theme styling and a
// fixed-capacity slot pool for callback storage. The pool size is set at
// compile time. Products that need more buttons override the macro via
// `target_compile_definitions`. Pool exhaustion is a hard error — the
// widget logs, asserts, and returns nullptr without creating any LVGL
// object, guaranteeing that no button is ever shipped with a dead
// callback.
//
// This file is the template every subsequent Camp 2 widget copies; the
// pattern (slot pool, LV_EVENT_DELETE release, setter discipline, theme
// tokens for every visual value) is the contract.

#ifndef BLUSYS_UI_BUTTON_POOL_SIZE
#define BLUSYS_UI_BUTTON_POOL_SIZE 32
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.button";

struct button_slot {
    press_cb_t on_press;
    void      *user_data;
    bool       in_use;
};

button_slot g_button_slots[BLUSYS_UI_BUTTON_POOL_SIZE] = {};

button_slot *acquire_slot()
{
    return detail::acquire_ui_slot(g_button_slots, kTag, "BLUSYS_UI_BUTTON_POOL_SIZE");
}

void release_slot(button_slot *slot)
{
    detail::release_ui_slot(slot);
}

lv_obj_t *button_label_child(lv_obj_t *button)
{
    if (lv_obj_get_child_count(button) == 0) {
        return nullptr;
    }
    return lv_obj_get_child(button, 0);
}

lv_color_t background_for(button_variant variant, const theme_tokens &t)
{
    switch (variant) {
        case button_variant::primary:   return t.color_primary;
        case button_variant::secondary: return t.color_surface_elevated;
        case button_variant::ghost:     return t.color_surface_elevated;
        case button_variant::danger:    return t.color_error;
    }
    return t.color_primary;
}

lv_color_t label_color_for(button_variant variant, const theme_tokens &t)
{
    switch (variant) {
        case button_variant::primary:
            return t.color_on_primary;
        case button_variant::secondary:
        case button_variant::ghost:
            return t.color_on_surface;
        case button_variant::danger:
            return t.color_on_error;
    }
    return t.color_on_primary;
}

void apply_variant(lv_obj_t *button, button_variant variant)
{
    const auto &t = theme();
    lv_obj_set_style_bg_color(button, background_for(variant, t), 0);
    lv_obj_set_style_bg_opa(button,
                            variant == button_variant::ghost ? LV_OPA_TRANSP : LV_OPA_COVER,
                            0);
    lv_obj_set_style_border_width(button,
                                  variant == button_variant::ghost ? 1 : 0,
                                  0);
    lv_obj_set_style_border_color(button, t.color_outline, 0);

    // Pressed state — darken via token-driven opacity.
    lv_obj_set_style_bg_opa(button, t.opa_pressed,
                            static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_PRESSED);

    // Disabled state — token-driven opacity.
    lv_obj_set_style_opa(button, t.opa_disabled,
                         static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DISABLED);

    // Focus ring.
    lv_obj_set_style_outline_width(button, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(button, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(button, 2, LV_STATE_FOCUSED);

    lv_obj_t *label = button_label_child(button);
    if (label != nullptr) {
        lv_obj_set_style_text_color(label, label_color_for(variant, t), 0);
    }
}

void on_lvgl_clicked(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<button_slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr && slot->on_press != nullptr) {
        slot->on_press(slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<button_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // namespace

lv_obj_t *button_create(lv_obj_t *parent, const button_config &config)
{
    // Acquire the slot up front so we fail before any LVGL object exists.
    button_slot *slot = nullptr;
    if (config.on_press != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
    }

    lv_obj_t   *button = lv_button_create(parent);
    const auto &t      = theme();

    apply_variant(button, config.variant);
    lv_obj_set_style_radius(button, t.radius_button, 0);
    lv_obj_set_style_pad_hor(button, t.spacing_md, 0);
    lv_obj_set_style_pad_ver(button, t.spacing_sm, 0);
    lv_obj_set_style_min_height(button, t.touch_target_min, 0);

    if (config.label != nullptr) {
        lv_obj_t *label = lv_label_create(button);
        lv_label_set_text(label, config.label);
        lv_obj_set_style_text_color(label, label_color_for(config.variant, t), 0);
        lv_obj_set_style_text_font(label, t.font_body, 0);
        lv_obj_set_style_text_letter_space(label, t.text_letter_space_body, 0);
        lv_obj_center(label);
    }

    if (slot != nullptr) {
        slot->on_press  = config.on_press;
        slot->user_data = config.user_data;
        lv_obj_set_user_data(button, slot);
        lv_obj_add_event_cb(button, on_lvgl_clicked, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(button, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);
    }

    detail::set_widget_disabled(button, config.disabled);

    return button;
}

void button_set_label(lv_obj_t *button, const char *label)
{
    if (button == nullptr) {
        return;
    }
    lv_obj_t *child = button_label_child(button);
    if (child == nullptr) {
        return;
    }
    lv_label_set_text(child, label != nullptr ? label : "");
}

void button_set_variant(lv_obj_t *button, button_variant variant)
{
    if (button == nullptr) {
        return;
    }
    apply_variant(button, variant);
}

void button_set_disabled(lv_obj_t *button, bool disabled)
{
    detail::set_widget_disabled(button, disabled);
}

}  // namespace blusys::ui
