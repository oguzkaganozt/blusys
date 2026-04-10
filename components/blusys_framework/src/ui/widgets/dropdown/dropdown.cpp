#include "blusys/framework/ui/widgets/dropdown/dropdown.hpp"

#include <cassert>
#include <cstring>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Themed dropdown selector wrapping `lv_dropdown`.
//
// LVGL's lv_dropdown expects a single '\n'-separated string for options.
// We accept a `const char * const *` array and join them at creation time.
// `LV_EVENT_VALUE_CHANGED` maps to `select_cb_t(index, user_data)`.
//
// Setter discipline: `dropdown_set_selected` does not fire the callback.

#ifndef BLUSYS_UI_DROPDOWN_POOL_SIZE
#define BLUSYS_UI_DROPDOWN_POOL_SIZE 16
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.dropdown";

struct dropdown_slot {
    select_cb_t  on_select;
    void        *user_data;
    bool         in_use;
};

dropdown_slot g_dropdown_slots[BLUSYS_UI_DROPDOWN_POOL_SIZE] = {};

dropdown_slot *acquire_slot()
{
    for (auto &s : g_dropdown_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_DROPDOWN_POOL_SIZE",
                BLUSYS_UI_DROPDOWN_POOL_SIZE);
    assert(false && "bu_dropdown slot pool exhausted");
    return nullptr;
}

void release_slot(dropdown_slot *slot)
{
    if (slot != nullptr) {
        slot->on_select = nullptr;
        slot->user_data = nullptr;
        slot->in_use    = false;
    }
}

// Join an array of strings with '\n' separators for lv_dropdown.
void set_options_from_array(lv_obj_t *dd, const char * const *options, int32_t count)
{
    if (options == nullptr || count <= 0) {
        lv_dropdown_set_options(dd, "");
        return;
    }

    // Build '\n'-separated string. Use a static buffer for simplicity;
    // lv_dropdown_set_options copies the string internally.
    static char buf[512];
    buf[0] = '\0';
    std::size_t pos = 0;

    for (int32_t i = 0; i < count; ++i) {
        const char *opt = options[i] != nullptr ? options[i] : "";
        std::size_t len = std::strlen(opt);
        if (i > 0 && pos < sizeof(buf) - 1) {
            buf[pos++] = '\n';
        }
        if (pos + len < sizeof(buf) - 1) {
            std::memcpy(buf + pos, opt, len);
            pos += len;
        }
    }
    buf[pos] = '\0';
    lv_dropdown_set_options(dd, buf);
}

void apply_theme(lv_obj_t *dd)
{
    const auto &t = theme();

    // Main button surface.
    lv_obj_set_style_bg_color(dd, t.color_surface, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(dd, t.color_outline, LV_PART_MAIN);
    lv_obj_set_style_border_width(dd, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(dd, t.radius_button, LV_PART_MAIN);
    lv_obj_set_style_text_color(dd, t.color_on_surface, LV_PART_MAIN);
    lv_obj_set_style_text_font(dd, t.font_body, LV_PART_MAIN);
    lv_obj_set_style_pad_left(dd, t.spacing_sm, LV_PART_MAIN);
    lv_obj_set_style_pad_right(dd, t.spacing_sm, LV_PART_MAIN);
    lv_obj_set_style_pad_top(dd, t.spacing_xs, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(dd, t.spacing_xs, LV_PART_MAIN);
    lv_obj_set_style_min_height(dd, t.touch_target_min, LV_PART_MAIN);

    // Focus ring.
    lv_obj_set_style_outline_width(dd, t.focus_ring_width, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(dd, t.color_focus_ring, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(dd, 2, LV_STATE_FOCUSED);

    // Disabled state.
    lv_obj_set_style_opa(dd, t.opa_disabled, LV_STATE_DISABLED);

    // Selected item highlight (applied when popup is open).
    lv_obj_set_style_bg_color(dd, t.color_primary,
                              static_cast<lv_style_selector_t>(LV_PART_SELECTED) | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(dd, t.color_on_primary,
                                static_cast<lv_style_selector_t>(LV_PART_SELECTED) | LV_STATE_CHECKED);
}

void on_lvgl_opened(lv_event_t *e)
{
    // Style the popup list when it opens — lv_dropdown_get_list() is only
    // valid while the popup is displayed.
    auto *dd   = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *list = lv_dropdown_get_list(dd);
    if (list == nullptr) {
        return;
    }
    const auto &t = theme();
    lv_obj_set_style_bg_color(list, t.color_surface, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(list, t.color_outline, LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(list, t.radius_card, LV_PART_MAIN);
    lv_obj_set_style_text_color(list, t.color_on_surface, LV_PART_MAIN);
    lv_obj_set_style_text_font(list, t.font_body, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(list, t.shadow_overlay_width, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(list, t.shadow_overlay_ofs_y, LV_PART_MAIN);
}

void on_lvgl_value_changed(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<dropdown_slot *>(lv_obj_get_user_data(obj));
    if (slot != nullptr && slot->on_select != nullptr) {
        int32_t index = static_cast<int32_t>(lv_dropdown_get_selected(obj));
        slot->on_select(index, slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<dropdown_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // namespace

lv_obj_t *dropdown_create(lv_obj_t *parent, const dropdown_config &config)
{
    dropdown_slot *slot = nullptr;
    if (config.on_select != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
        slot->on_select = config.on_select;
        slot->user_data = config.user_data;
    }

    lv_obj_t *dd = lv_dropdown_create(parent);
    set_options_from_array(dd, config.options, config.option_count);
    lv_dropdown_set_selected(dd, static_cast<uint32_t>(config.initial));

    apply_theme(dd);

    // Style the popup list whenever it opens.
    lv_obj_add_event_cb(dd, on_lvgl_opened, LV_EVENT_READY, nullptr);

    if (slot != nullptr) {
        lv_obj_set_user_data(dd, slot);
        lv_obj_add_event_cb(dd, on_lvgl_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_obj_add_event_cb(dd, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);
    }

    if (config.disabled) {
        lv_obj_add_state(dd, LV_STATE_DISABLED);
    }

    return dd;
}

void dropdown_set_selected(lv_obj_t *dropdown, int32_t index)
{
    if (dropdown == nullptr) {
        return;
    }
    lv_dropdown_set_selected(dropdown, static_cast<uint32_t>(index));
}

int32_t dropdown_get_selected(lv_obj_t *dropdown)
{
    if (dropdown == nullptr) {
        return 0;
    }
    return static_cast<int32_t>(lv_dropdown_get_selected(dropdown));
}

void dropdown_set_options(lv_obj_t *dropdown, const char * const *options, int32_t count)
{
    if (dropdown == nullptr) {
        return;
    }
    set_options_from_array(dropdown, options, count);
}

void dropdown_set_disabled(lv_obj_t *dropdown, bool disabled)
{
    if (dropdown == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(dropdown, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(dropdown, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui
