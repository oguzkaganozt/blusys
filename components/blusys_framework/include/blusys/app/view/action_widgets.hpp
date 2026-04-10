#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"
#include "blusys/framework/ui/primitives/row.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/framework/ui/widgets/slider/slider.hpp"
#include "blusys/framework/ui/widgets/toggle/toggle.hpp"
#include "blusys/log.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace blusys::app::view {

// ---- internal: type-erased action dispatch slot pool ----

namespace detail {

#ifndef BLUSYS_APP_ACTION_SLOT_POOL_SIZE
#define BLUSYS_APP_ACTION_SLOT_POOL_SIZE 32
#endif

// Maximum size of an Action struct stored in a slot. 16 bytes covers
// typical tagged-union actions. If your Action is larger, increase this.
static constexpr std::size_t kActionStorageSize = 16;

struct action_dispatch_slot {
    app_ctx *ctx;
    void (*dispatch_fn)(app_ctx *ctx, const void *action_data);
    alignas(8) char action_storage[kActionStorageSize];
    bool in_use;
};

action_dispatch_slot *acquire_action_slot();
void release_action_slot(action_dispatch_slot *slot);

// Register an LV_EVENT_DELETE handler that releases the action slot
// when the LVGL widget is destroyed. Must be called after widget creation.
void attach_slot_cleanup(lv_obj_t *widget, action_dispatch_slot *slot);

// Typed dispatch trampoline: casts storage back to Action and dispatches.
template <typename Action>
void typed_dispatch(app_ctx *ctx, const void *action_data)
{
    const auto &action = *static_cast<const Action *>(action_data);
    ctx->dispatch(action);
}

// Helper: acquire a slot, store the action, and set the trampoline.
template <typename Action>
action_dispatch_slot *make_action_slot(const Action &action, app_ctx *ctx)
{
    static_assert(sizeof(Action) <= kActionStorageSize,
                  "Action too large for action_dispatch_slot storage");
    static_assert(alignof(Action) <= 8,
                  "Action alignment exceeds slot alignment");

    action_dispatch_slot *slot = acquire_action_slot();
    if (slot == nullptr) {
        return nullptr;
    }

    slot->ctx = ctx;
    slot->dispatch_fn = &typed_dispatch<Action>;
    std::memcpy(slot->action_storage, &action, sizeof(Action));
    return slot;
}

}  // namespace detail

// ---- action-bound button ----
//
// Dispatches the given action through app_ctx on press.
// Eliminates the DispatchBridge boilerplate from Phase 1.

template <typename Action>
lv_obj_t *button(lv_obj_t *parent,
                 const char *label,
                 const Action &action,
                 app_ctx *ctx,
                 blusys::ui::button_variant variant = blusys::ui::button_variant::primary,
                 bool disabled = false)
{
    auto *slot = detail::make_action_slot(action, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for button");
        return nullptr;
    }

    lv_obj_t *widget = blusys::ui::button_create(parent, {
        .label    = label,
        .variant  = variant,
        .on_press = +[](void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            s->dispatch_fn(s->ctx, s->action_storage);
        },
        .user_data = slot,
        .disabled  = disabled,
    });
    if (widget != nullptr) {
        detail::attach_slot_cleanup(widget, slot);
    } else {
        detail::release_action_slot(slot);
    }
    return widget;
}

// ---- action-bound toggle ----
//
// Dispatches on_action when toggled on, off_action when toggled off.

template <typename Action>
lv_obj_t *toggle(lv_obj_t *parent,
                 bool initial,
                 const Action &on_action,
                 const Action &off_action,
                 app_ctx *ctx,
                 bool disabled = false)
{
    // We need two actions but only one slot. Store both by using a
    // pair struct that fits in kActionStorageSize.
    struct toggle_pair {
        Action on_act;
        Action off_act;
    };
    static_assert(sizeof(toggle_pair) <= detail::kActionStorageSize,
                  "Toggle action pair too large for slot storage");

    toggle_pair pair{on_action, off_action};
    auto *slot = detail::make_action_slot(pair, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for toggle");
        return nullptr;
    }

    // Override the dispatch_fn — toggle needs the bool state to pick the right action.
    slot->dispatch_fn = nullptr;  // not used directly

    lv_obj_t *widget = blusys::ui::toggle_create(parent, {
        .initial   = initial,
        .on_change = +[](bool new_state, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &p = *reinterpret_cast<const toggle_pair *>(s->action_storage);
            const auto &action = new_state ? p.on_act : p.off_act;
            s->ctx->dispatch(action);
        },
        .user_data = slot,
        .disabled  = disabled,
    });
    if (widget != nullptr) {
        detail::attach_slot_cleanup(widget, slot);
    } else {
        detail::release_action_slot(slot);
    }
    return widget;
}

// ---- action-bound slider ----
//
// Calls maker(new_value) to produce an Action, then dispatches it.

template <typename Action>
lv_obj_t *slider(lv_obj_t *parent,
                 std::int32_t min,
                 std::int32_t max,
                 std::int32_t initial,
                 Action (*maker)(std::int32_t value),
                 app_ctx *ctx,
                 bool disabled = false)
{
    // Store the maker function pointer + ctx in the slot.
    struct slider_data {
        Action (*maker)(std::int32_t);
        app_ctx *ctx;
    };
    static_assert(sizeof(slider_data) <= detail::kActionStorageSize,
                  "Slider data too large for slot storage");

    slider_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for slider");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;  // not used directly

    lv_obj_t *widget = blusys::ui::slider_create(parent, {
        .min       = min,
        .max       = max,
        .initial   = initial,
        .on_change = +[](std::int32_t new_value, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const slider_data *>(s->action_storage);
            auto action = d.maker(new_value);
            d.ctx->dispatch(action);
        },
        .user_data = slot,
        .disabled  = disabled,
    });
    if (widget != nullptr) {
        detail::attach_slot_cleanup(widget, slot);
    } else {
        detail::release_action_slot(slot);
    }
    return widget;
}

// ---- convenience: label and divider (non-interactive, pass-through) ----

inline lv_obj_t *label(lv_obj_t *parent, const char *text,
                       const lv_font_t *font = nullptr)
{
    return blusys::ui::label_create(parent, {
        .text = text,
        .font = font != nullptr ? font : blusys::ui::theme().font_body,
    });
}

inline lv_obj_t *title(lv_obj_t *parent, const char *text)
{
    return blusys::ui::label_create(parent, {
        .text = text,
        .font = blusys::ui::theme().font_title,
    });
}

inline lv_obj_t *divider(lv_obj_t *parent)
{
    return blusys::ui::divider_create(parent, {});
}

inline lv_obj_t *row(lv_obj_t *parent, int gap = -1, int padding = 0)
{
    return blusys::ui::row_create(parent, {
        .gap     = (gap >= 0) ? gap : blusys::ui::theme().spacing_sm,
        .padding = padding,
    });
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
