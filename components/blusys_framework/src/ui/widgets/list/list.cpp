#include "blusys/framework/ui/widgets/list/list.hpp"

#include <cassert>

#include "blusys/framework/ui/icons/icon_set.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Scrollable selectable list wrapping raw `lv_obj` containers.
//
// Internal layout: the outer container is scrollable with vertical flex.
// Each item is a clickable child row containing an optional icon label,
// primary text, and optional secondary text. `LV_EVENT_CLICKED` on a
// row fires `select_cb_t(index, user_data)`.
//
// The slot pool stores callbacks. Programmatic `list_set_selected`
// highlights the row but does not fire the callback.

#ifndef BLUSYS_UI_LIST_POOL_SIZE
#define BLUSYS_UI_LIST_POOL_SIZE 16
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.list";

struct list_slot {
    select_cb_t  on_select;
    void        *user_data;
    int32_t      selected;
    bool         in_use;
};

list_slot g_list_slots[BLUSYS_UI_LIST_POOL_SIZE] = {};

list_slot *acquire_slot()
{
    for (auto &s : g_list_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_LIST_POOL_SIZE",
                BLUSYS_UI_LIST_POOL_SIZE);
    assert(false && "bu_list slot pool exhausted");
    return nullptr;
}

void release_slot(list_slot *slot)
{
    if (slot != nullptr) {
        slot->on_select = nullptr;
        slot->user_data = nullptr;
        slot->selected  = -1;
        slot->in_use    = false;
    }
}

void highlight_row(lv_obj_t *list, int32_t index)
{
    const auto &t = theme();
    uint32_t count = lv_obj_get_child_count(list);
    for (uint32_t i = 0; i < count; ++i) {
        lv_obj_t *row = lv_obj_get_child(list, static_cast<int32_t>(i));
        if (static_cast<int32_t>(i) == index) {
            lv_obj_set_style_bg_color(row, t.color_primary, 0);
            lv_obj_set_style_bg_opa(row, t.opa_focused, 0);
        } else {
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        }
    }
}

void on_row_clicked(lv_event_t *e)
{
    auto *row  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *list = lv_obj_get_parent(row);
    auto *slot = static_cast<list_slot *>(lv_obj_get_user_data(list));
    if (slot == nullptr || slot->on_select == nullptr) {
        return;
    }

    // Determine index of clicked row.
    int32_t index = lv_obj_get_index(row);
    slot->selected = index;
    highlight_row(list, index);
    slot->on_select(index, slot->user_data);
}

void on_list_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<list_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

void build_items(lv_obj_t *list, const list_item *items, int32_t count,
                 bool has_callback)
{
    const auto &t = theme();

    for (int32_t i = 0; i < count; ++i) {
        const auto &item = items[i];

        // Row container.
        lv_obj_t *row = lv_obj_create(list);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_left(row, t.spacing_sm, 0);
        lv_obj_set_style_pad_right(row, t.spacing_sm, 0);
        lv_obj_set_style_pad_top(row, t.spacing_sm, 0);
        lv_obj_set_style_pad_bottom(row, t.spacing_sm, 0);
        lv_obj_set_style_pad_column(row, t.spacing_sm, 0);
        lv_obj_set_style_radius(row, t.radius_button, 0);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_min_height(row, t.touch_target_min, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // Make clickable for focus and selection.
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

        // Focus ring.
        lv_obj_set_style_outline_width(row, t.focus_ring_width, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(row, t.color_focus_ring, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(row, 2, LV_STATE_FOCUSED);

        // Pressed opacity.
        lv_obj_set_style_bg_opa(row, t.opa_pressed, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(row, t.color_primary, LV_STATE_PRESSED);

        // Optional icon.
        if (item.icon != nullptr) {
            lv_obj_t *icon = lv_label_create(row);
            const lv_font_t *ifont =
                (t.icons != nullptr && t.icons->font != nullptr) ? t.icons->font : t.font_body;
            lv_obj_set_style_text_font(icon, ifont, 0);
            lv_obj_set_style_text_color(icon, t.color_on_surface, 0);
            lv_label_set_text(icon, item.icon);
        }

        // Text column (primary + optional secondary).
        lv_obj_t *text_col = lv_obj_create(row);
        lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(text_col, 0, 0);
        lv_obj_set_style_pad_all(text_col, 0, 0);
        lv_obj_set_style_pad_row(text_col, 2, 0);
        lv_obj_set_width(text_col, LV_PCT(100));
        lv_obj_set_height(text_col, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(text_col, 1);
        lv_obj_remove_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(text_col, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *primary = lv_label_create(text_col);
        lv_obj_set_style_text_font(primary, t.font_body, 0);
        lv_obj_set_style_text_color(primary, t.color_on_surface, 0);
        lv_label_set_text(primary, item.text != nullptr ? item.text : "");

        if (item.secondary != nullptr) {
            lv_obj_t *secondary = lv_label_create(text_col);
            lv_obj_set_style_text_font(secondary, t.font_body_sm, 0);
            lv_obj_set_style_text_color(secondary, t.color_on_surface, 0);
            lv_obj_set_style_text_opa(secondary, t.opa_disabled, 0);
            lv_label_set_text(secondary, item.secondary);
        }

        // Click handler on row.
        if (has_callback) {
            lv_obj_add_event_cb(row, on_row_clicked, LV_EVENT_CLICKED, nullptr);
        }
    }
}

}  // namespace

lv_obj_t *list_create(lv_obj_t *parent, const list_config &config)
{
    list_slot *slot = nullptr;
    if (config.on_select != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
        slot->on_select = config.on_select;
        slot->user_data = config.user_data;
        slot->selected  = -1;
    }

    const auto &t = theme();

    // Scrollable container.
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, t.spacing_xs, 0);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    if (slot != nullptr) {
        lv_obj_set_user_data(list, slot);
        lv_obj_add_event_cb(list, on_list_deleted, LV_EVENT_DELETE, nullptr);
    }

    if (config.items != nullptr && config.item_count > 0) {
        build_items(list, config.items, config.item_count, config.on_select != nullptr);
    }

    if (config.disabled) {
        lv_obj_add_state(list, LV_STATE_DISABLED);
    }

    return list;
}

void list_set_items(lv_obj_t *list, const list_item *items, int32_t count)
{
    if (list == nullptr) {
        return;
    }
    lv_obj_clean(list);
    auto *slot = static_cast<list_slot *>(lv_obj_get_user_data(list));
    bool has_cb = (slot != nullptr && slot->on_select != nullptr);
    if (slot != nullptr) {
        slot->selected = -1;
    }
    if (items != nullptr && count > 0) {
        build_items(list, items, count, has_cb);
    }
}

void list_set_selected(lv_obj_t *list, int32_t index)
{
    if (list == nullptr) {
        return;
    }
    auto *slot = static_cast<list_slot *>(lv_obj_get_user_data(list));
    if (slot != nullptr) {
        slot->selected = index;
    }
    highlight_row(list, index);
}

int32_t list_get_selected(lv_obj_t *list)
{
    if (list == nullptr) {
        return -1;
    }
    auto *slot = static_cast<list_slot *>(lv_obj_get_user_data(list));
    return slot != nullptr ? slot->selected : -1;
}

void list_set_disabled(lv_obj_t *list, bool disabled)
{
    if (list == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(list, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(list, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui
