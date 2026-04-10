#include "blusys/framework/ui/widgets/overlay/overlay.hpp"

#include <cassert>
#include <cstdint>

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

// Composition implementation: a compact themed container anchored to the
// bottom of its parent, with an `lv_label` child rendering the text. The
// auto-dismiss behaviour is driven by an `lv_timer` created on
// `overlay_show` and destroyed either on timer expiry, manual hide, or
// `LV_EVENT_DELETE`.
//
// Unlike the other Camp 2 widgets, the slot is **always** allocated:
// the slot must hold the active `lv_timer_t *` and the duration so we
// can manage the timer lifecycle correctly even when the product
// passes no `on_hidden` callback. Pool exhaustion is still a hard
// fail-loud error.
//
// Timer lifecycle invariants:
//   • `overlay_show` cancels any pre-existing timer before starting a
//     new one (re-show always restarts the dismiss countdown).
//   • The timer is one-shot via `lv_timer_set_repeat_count(t, 1)`. LVGL
//     auto-deletes one-shot timers after they fire, so the callback
//     just clears `slot->timer = nullptr` instead of deleting it.
//   • `overlay_hide` and `LV_EVENT_DELETE` both call `cancel_timer`
//     which only deletes the timer if it is still active.

#ifndef BLUSYS_UI_OVERLAY_POOL_SIZE
#define BLUSYS_UI_OVERLAY_POOL_SIZE 8
#endif

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.overlay";

struct overlay_slot {
    press_cb_t  on_hidden;
    void       *user_data;
    lv_obj_t   *overlay;       // back-pointer for the timer callback
    lv_timer_t *timer;
    uint32_t    duration_ms;
    bool        in_use;
};

overlay_slot g_overlay_slots[BLUSYS_UI_OVERLAY_POOL_SIZE] = {};

overlay_slot *acquire_slot()
{
    for (auto &s : g_overlay_slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(kTag,
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_OVERLAY_POOL_SIZE",
                BLUSYS_UI_OVERLAY_POOL_SIZE);
    assert(false && "bu_overlay slot pool exhausted");
    return nullptr;
}

void release_slot(overlay_slot *slot)
{
    if (slot != nullptr) {
        slot->on_hidden   = nullptr;
        slot->user_data   = nullptr;
        slot->overlay     = nullptr;
        slot->timer       = nullptr;
        slot->duration_ms = 0;
        slot->in_use      = false;
    }
}

void cancel_timer(overlay_slot *slot)
{
    if (slot != nullptr && slot->timer != nullptr) {
        lv_timer_delete(slot->timer);
        slot->timer = nullptr;
    }
}

void style_overlay(lv_obj_t *overlay)
{
    const auto &t = theme();
    lv_obj_set_width(overlay, LV_SIZE_CONTENT);
    lv_obj_set_height(overlay, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(overlay, LV_PCT(80), 0);
    lv_obj_set_style_bg_color(overlay, t.color_primary, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, t.radius_button, 0);
    lv_obj_set_style_pad_all(overlay, t.spacing_md, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Shadow from tokens.
    lv_obj_set_style_shadow_width(overlay, t.shadow_overlay_width, 0);
    lv_obj_set_style_shadow_offset_y(overlay, t.shadow_overlay_ofs_y, 0);
    lv_obj_set_style_shadow_color(overlay, t.shadow_color, 0);
    lv_obj_set_style_shadow_opa(overlay, LV_OPA_40, 0);
}

void on_timer(lv_timer_t *t)
{
    auto *slot = static_cast<overlay_slot *>(lv_timer_get_user_data(t));
    if (slot == nullptr) {
        return;
    }
    // One-shot timer: LVGL auto-deletes after the callback returns, so
    // we only need to clear our reference.
    slot->timer = nullptr;
    if (slot->overlay != nullptr) {
        lv_obj_add_flag(slot->overlay, LV_OBJ_FLAG_HIDDEN);
    }
    if (slot->on_hidden != nullptr) {
        slot->on_hidden(slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t *e)
{
    auto *obj  = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *slot = static_cast<overlay_slot *>(lv_obj_get_user_data(obj));
    cancel_timer(slot);
    release_slot(slot);
}

}  // namespace

lv_obj_t *overlay_create(lv_obj_t *parent, const overlay_config &config)
{
    overlay_slot *slot = acquire_slot();
    if (slot == nullptr) {
        return nullptr;
    }

    lv_obj_t *overlay = lv_obj_create(parent);
    style_overlay(overlay);
    lv_obj_align(overlay, LV_ALIGN_BOTTOM_MID, 0, -theme().spacing_lg);

    if (config.text != nullptr) {
        const auto &t = theme();
        lv_obj_t   *label = lv_label_create(overlay);
        lv_label_set_text(label, config.text);
        lv_obj_set_style_text_color(label, t.color_on_primary, 0);
        lv_obj_set_style_text_font(label, t.font_body, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_center(label);
    }

    slot->on_hidden   = config.on_hidden;
    slot->user_data   = config.user_data;
    slot->overlay     = overlay;
    slot->timer       = nullptr;
    slot->duration_ms = config.duration_ms;

    lv_obj_set_user_data(overlay, slot);
    lv_obj_add_event_cb(overlay, on_lvgl_deleted, LV_EVENT_DELETE, nullptr);

    // Created hidden by convention; product calls overlay_show when ready.
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);

    return overlay;
}

void overlay_show(lv_obj_t *overlay)
{
    if (overlay == nullptr) {
        return;
    }
    auto *slot = static_cast<overlay_slot *>(lv_obj_get_user_data(overlay));

    // Re-show always restarts the dismiss countdown.
    cancel_timer(slot);

    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_to_index(overlay, -1);

    if (slot != nullptr && slot->duration_ms > 0) {
        slot->timer = lv_timer_create(on_timer, slot->duration_ms, slot);
        if (slot->timer != nullptr) {
            lv_timer_set_repeat_count(slot->timer, 1);
        }
    }
}

void overlay_hide(lv_obj_t *overlay)
{
    if (overlay == nullptr) {
        return;
    }
    auto *slot = static_cast<overlay_slot *>(lv_obj_get_user_data(overlay));
    const bool was_visible = !lv_obj_has_flag(overlay, LV_OBJ_FLAG_HIDDEN);

    cancel_timer(slot);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);

    if (was_visible && slot != nullptr && slot->on_hidden != nullptr) {
        slot->on_hidden(slot->user_data);
    }
}

bool overlay_is_visible(lv_obj_t *overlay)
{
    if (overlay == nullptr) {
        return false;
    }
    return !lv_obj_has_flag(overlay, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace blusys::ui
