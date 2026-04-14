#include "blusys/framework/ui/widgets/tabs.hpp"

#include "../fixed_slot_pool.hpp"
#include "../widget_common.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/hal/log.h"

// Content-level tab switcher. Distinct from the shell tab_bar.
//
// Internal layout: a vertical container with a horizontal button strip
// at the top and a stack of content panels below. Only the active panel
// is visible. Tab button clicks switch the active panel and fire
// `select_cb_t(index, user_data)`.
//
// Products call `tabs_get_content(tabs, index)` to get each tab's
// content container and populate it with children.

#ifndef BLUSYS_UI_TABS_POOL_SIZE
#define BLUSYS_UI_TABS_POOL_SIZE 8
#endif

namespace blusys {
namespace {

constexpr const char *kTag = "ui.tabs";

struct tabs_slot {
    select_cb_t  on_change;
    void        *user_data;
    int32_t      active;
    int32_t      count;
    bool         in_use;
};

detail::slot_pool<tabs_slot, BLUSYS_UI_TABS_POOL_SIZE> g_tabs_pool{
    "ui.tabs", "BLUSYS_UI_TABS_POOL_SIZE"};

// The outer container has two children:
//   child 0 = tab strip (horizontal row of buttons)
//   child 1 = content stack (holds N content panels)
lv_obj_t *strip_from(lv_obj_t *tabs) { return lv_obj_get_child(tabs, 0); }
lv_obj_t *stack_from(lv_obj_t *tabs) { return lv_obj_get_child(tabs, 1); }

void apply_strip_button_active(lv_obj_t *btn, bool active)
{
    const auto &t = theme();
    if (active) {
        lv_obj_set_style_bg_color(btn, t.color_primary, 0);
        lv_obj_set_style_bg_opa(btn, t.opa_surface_subtle, 0);
        // Indicator line at bottom.
        lv_obj_set_style_border_color(btn, t.color_primary, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, t.color_primary, 0);
    } else {
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, t.color_on_surface, 0);
    }
}

void activate_tab(lv_obj_t *tabs, tabs_slot *slot, int32_t index)
{
    if (index < 0 || index >= slot->count) {
        return;
    }

    slot->active = index;

    // Update strip buttons.
    lv_obj_t *strip = strip_from(tabs);
    uint32_t btn_count = lv_obj_get_child_count(strip);
    for (uint32_t i = 0; i < btn_count; ++i) {
        apply_strip_button_active(lv_obj_get_child(strip, static_cast<int32_t>(i)),
                                  static_cast<int32_t>(i) == index);
    }

    // Show only the active content panel.
    lv_obj_t *stack = stack_from(tabs);
    uint32_t panel_count = lv_obj_get_child_count(stack);
    for (uint32_t i = 0; i < panel_count; ++i) {
        lv_obj_t *panel = lv_obj_get_child(stack, static_cast<int32_t>(i));
        if (static_cast<int32_t>(i) == index) {
            lv_obj_remove_flag(panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void on_tab_clicked(lv_event_t *e)
{
    auto *btn  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *strip = lv_obj_get_parent(btn);
    auto *tabs  = lv_obj_get_parent(strip);
    auto *slot  = static_cast<tabs_slot *>(lv_obj_get_user_data(tabs));
    if (slot == nullptr) {
        return;
    }

    int32_t index = lv_obj_get_index(btn);
    if (index == slot->active) {
        return;  // already active
    }

    activate_tab(tabs, slot, index);

    if (slot->on_change != nullptr) {
        slot->on_change(index, slot->user_data);
    }
}

}  // namespace

lv_obj_t *tabs_create(lv_obj_t *parent, const tabs_config &config)
{
    int32_t count = config.item_count;
    if (count <= 0 || count > kMaxContentTabs || config.items == nullptr) {
        BLUSYS_LOGE(kTag, "invalid tabs config: items=%p count=%d",
                    static_cast<const void *>(config.items), static_cast<int>(count));
        return nullptr;
    }

    tabs_slot *slot = g_tabs_pool.acquire();
    if (slot == nullptr) {
        return nullptr;
    }
    slot->on_change = config.on_change;
    slot->user_data = config.user_data;
    slot->active    = config.initial;
    slot->count     = count;

    const auto &t = theme();

    // Outer container.
    lv_obj_t *tabs = lv_obj_create(parent);
    lv_obj_set_layout(tabs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(tabs, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tabs, 0, 0);
    lv_obj_set_style_pad_all(tabs, 0, 0);
    lv_obj_set_style_pad_row(tabs, 0, 0);
    lv_obj_set_width(tabs, LV_PCT(100));
    lv_obj_set_flex_grow(tabs, 1);
    lv_obj_remove_flag(tabs, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(tabs, slot);
    lv_obj_add_event_cb(tabs, detail::release_slot_on_delete<tabs_slot>,
                        LV_EVENT_DELETE, nullptr);

    // Tab strip — horizontal row of buttons.
    lv_obj_t *strip = lv_obj_create(tabs);
    lv_obj_set_layout(strip, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(strip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(strip, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(strip, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(strip, 0, 0);
    lv_obj_set_style_pad_all(strip, 0, 0);
    lv_obj_set_style_pad_column(strip, 0, 0);
    lv_obj_set_width(strip, LV_PCT(100));
    lv_obj_set_height(strip, LV_SIZE_CONTENT);
    lv_obj_remove_flag(strip, LV_OBJ_FLAG_SCROLLABLE);

    // Separator line below strip.
    lv_obj_set_style_border_color(strip, t.color_outline_variant, LV_PART_MAIN);
    lv_obj_set_style_border_width(strip, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(strip, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    for (int32_t i = 0; i < count; ++i) {
        lv_obj_t *btn = lv_obj_create(strip);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_left(btn, t.spacing_md, 0);
        lv_obj_set_style_pad_right(btn, t.spacing_md, 0);
        lv_obj_set_style_pad_top(btn, t.spacing_sm, 0);
        lv_obj_set_style_pad_bottom(btn, t.spacing_sm, 0);
        lv_obj_set_height(btn, LV_SIZE_CONTENT);
        lv_obj_set_width(btn, LV_SIZE_CONTENT);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        // Focus ring.
        lv_obj_set_style_outline_width(btn, t.focus_ring_width, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(btn, t.color_focus_ring, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(btn, 2, LV_STATE_FOCUSED);

        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_style_text_font(label, t.font_label, 0);
        lv_label_set_text(label, config.items[i].label != nullptr ? config.items[i].label : "");

        lv_obj_add_event_cb(btn, on_tab_clicked, LV_EVENT_CLICKED, nullptr);
    }

    // Content stack — one panel per tab, only the active one visible.
    lv_obj_t *stack = lv_obj_create(tabs);
    lv_obj_set_layout(stack, LV_LAYOUT_NONE);
    lv_obj_set_style_bg_opa(stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stack, 0, 0);
    lv_obj_set_style_pad_all(stack, 0, 0);
    lv_obj_set_width(stack, LV_PCT(100));
    lv_obj_set_flex_grow(stack, 1);
    lv_obj_remove_flag(stack, LV_OBJ_FLAG_SCROLLABLE);

    for (int32_t i = 0; i < count; ++i) {
        lv_obj_t *panel = lv_obj_create(stack);
        lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_set_style_pad_all(panel, t.spacing_sm, 0);
        lv_obj_set_style_pad_row(panel, t.spacing_sm, 0);
        lv_obj_set_width(panel, LV_PCT(100));
        lv_obj_set_height(panel, LV_PCT(100));
        lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        if (i != config.initial) {
            lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Apply active state to strip buttons.
    activate_tab(tabs, slot, config.initial);

    return tabs;
}

void tabs_set_active(lv_obj_t *tabs, int32_t index)
{
    if (tabs == nullptr) {
        return;
    }
    auto *slot = static_cast<tabs_slot *>(lv_obj_get_user_data(tabs));
    if (slot == nullptr) {
        return;
    }
    activate_tab(tabs, slot, index);
    // Setter discipline: no callback.
}

int32_t tabs_get_active(lv_obj_t *tabs)
{
    if (tabs == nullptr) {
        return 0;
    }
    auto *slot = static_cast<tabs_slot *>(lv_obj_get_user_data(tabs));
    return slot != nullptr ? slot->active : 0;
}

lv_obj_t *tabs_get_content(lv_obj_t *tabs, int32_t index)
{
    if (tabs == nullptr) {
        return nullptr;
    }
    lv_obj_t *stack = stack_from(tabs);
    if (stack == nullptr) {
        return nullptr;
    }
    auto *slot = static_cast<tabs_slot *>(lv_obj_get_user_data(tabs));
    if (slot == nullptr || index < 0 || index >= slot->count) {
        return nullptr;
    }
    return lv_obj_get_child(stack, index);
}

}  // namespace blusys
