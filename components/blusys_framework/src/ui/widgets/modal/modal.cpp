#include "blusys/framework/ui/widgets/modal/modal.hpp"

#include <cassert>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Composition implementation: a backdrop `lv_obj` (full screen, dim) with
// a centered panel child holding optional title and body labels. This is
// not a single stock LVGL widget wrap, but it follows the same six-rule
// contract: theme tokens for every visual value, semantic config struct,
// setters for state, slot-pool callback storage, fail-loud on exhaustion.
//
// Click-to-dismiss uses LVGL event bubbling: the handler is registered
// on the backdrop, and `lv_event_get_target` vs `lv_event_get_current_target`
// distinguish backdrop clicks from clicks bubbled up from the panel.
// Only true backdrop clicks fire `on_dismiss`.

#ifndef BLUSYS_UI_MODAL_POOL_SIZE
#define BLUSYS_UI_MODAL_POOL_SIZE 8
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.modal";

struct modal_slot {
    press_cb_t on_dismiss;
    void      *user_data;
    bool       in_use;
};

modal_slot g_modal_slots[BLUSYS_UI_MODAL_POOL_SIZE] = {};

modal_slot *acquire_slot()
{
    for (auto &s : g_modal_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_MODAL_POOL_SIZE",
                BLUSYS_UI_MODAL_POOL_SIZE);
    assert(false && "bu_modal slot pool exhausted");
    return nullptr;
}

void release_slot(modal_slot *slot)
{
    if (slot != nullptr) {
        slot->on_dismiss = nullptr;
        slot->user_data  = nullptr;
        slot->in_use     = false;
    }
}

void style_backdrop(lv_obj_t *backdrop)
{
    const auto &t = theme();
    lv_obj_set_size(backdrop, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(backdrop, t.color_background, 0);
    lv_obj_set_style_bg_opa(backdrop, t.opa_backdrop, 0);
    lv_obj_set_style_border_width(backdrop, 0, 0);
    lv_obj_set_style_radius(backdrop, 0, 0);
    lv_obj_set_style_pad_all(backdrop, 0, 0);
    lv_obj_set_layout(backdrop, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(backdrop, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(backdrop,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);
}

void style_panel(lv_obj_t *panel)
{
    const auto &t = theme();
    lv_obj_set_width(panel, LV_PCT(80));
    lv_obj_set_height(panel, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(panel, t.color_surface, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, t.color_outline, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, t.radius_card, 0);
    lv_obj_set_style_pad_all(panel, t.spacing_lg, 0);
    lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, t.spacing_md, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // Shadow from tokens.
    lv_obj_set_style_shadow_width(panel, t.shadow_overlay_width, 0);
    lv_obj_set_style_shadow_offset_y(panel, t.shadow_overlay_ofs_y, 0);
    lv_obj_set_style_shadow_color(panel, t.shadow_color, 0);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_40, 0);
}

void on_lvgl_clicked(lv_event_t *e)
{
    auto *target  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *current = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
    // Only fire dismiss when the click hits the backdrop itself, not when
    // it bubbles up from the panel or one of its descendants.
    if (target != current) {
        return;
    }
    auto *slot = static_cast<modal_slot *>(lv_obj_get_user_data(current));
    if (slot != nullptr && slot->on_dismiss != nullptr) {
        slot->on_dismiss(slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<modal_slot *>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // namespace

lv_obj_t *modal_create(lv_obj_t *parent, const modal_config &config)
{
    modal_slot *slot = nullptr;
    if (config.on_dismiss != nullptr) {
        slot = acquire_slot();
        if (slot == nullptr) {
            return nullptr;
        }
    }

    lv_obj_t *backdrop = lv_obj_create(parent);
    style_backdrop(backdrop);

    lv_obj_t *panel = lv_obj_create(backdrop);
    style_panel(panel);

    const auto &t = theme();

    if (config.title != nullptr) {
        lv_obj_t *title_label = lv_label_create(panel);
        lv_label_set_text(title_label, config.title);
        lv_obj_set_style_text_color(title_label, t.color_on_surface, 0);
        lv_obj_set_style_text_font(title_label, t.font_title, 0);
    }
    if (config.body != nullptr) {
        lv_obj_t *body_label = lv_label_create(panel);
        lv_label_set_text(body_label, config.body);
        lv_obj_set_style_text_color(body_label, t.color_on_surface, 0);
        lv_obj_set_style_text_font(body_label, t.font_body, 0);
        lv_obj_set_width(body_label, LV_PCT(100));
        lv_label_set_long_mode(body_label, LV_LABEL_LONG_WRAP);
    }

    if (slot != nullptr) {
        slot->on_dismiss = config.on_dismiss;
        slot->user_data  = config.user_data;
        lv_obj_set_user_data(backdrop, slot);
        lv_obj_add_flag(backdrop, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(backdrop, on_lvgl_clicked, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(backdrop, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);
    }

    // Created hidden by convention; product calls modal_show when ready.
    lv_obj_add_flag(backdrop, LV_OBJ_FLAG_HIDDEN);

    return backdrop;
}

void modal_show(lv_obj_t *modal)
{
    if (modal == nullptr) {
        return;
    }
    lv_obj_remove_flag(modal, LV_OBJ_FLAG_HIDDEN);
    // Bring to front in the parent's child stack so it covers siblings.
    // LVGL 9 convention: index 0 is background, -1 is foreground.
    lv_obj_move_to_index(modal, -1);
}

void modal_hide(lv_obj_t *modal)
{
    if (modal == nullptr) {
        return;
    }
    lv_obj_add_flag(modal, LV_OBJ_FLAG_HIDDEN);
}

bool modal_is_visible(lv_obj_t *modal)
{
    if (modal == nullptr) {
        return false;
    }
    return !lv_obj_has_flag(modal, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace blusys::ui
