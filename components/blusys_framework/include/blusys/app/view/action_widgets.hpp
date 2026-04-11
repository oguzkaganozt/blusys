#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"
#include "blusys/framework/ui/primitives/icon_label.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/row.hpp"
#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/framework/ui/widgets/card/card.hpp"
#include "blusys/framework/ui/widgets/chart/chart.hpp"
#include "blusys/framework/ui/widgets/data_table/data_table.hpp"
#include "blusys/framework/ui/widgets/dropdown/dropdown.hpp"
#include "blusys/framework/ui/widgets/gauge/gauge.hpp"
#include "blusys/framework/ui/widgets/input_field/input_field.hpp"
#include "blusys/framework/ui/widgets/knob/knob.hpp"
#include "blusys/framework/ui/widgets/level_bar/level_bar.hpp"
#include "blusys/framework/ui/widgets/list/list.hpp"
#include "blusys/framework/ui/widgets/progress/progress.hpp"
#include "blusys/framework/ui/widgets/slider/slider.hpp"
#include "blusys/framework/ui/widgets/tabs/tabs.hpp"
#include "blusys/framework/ui/widgets/toggle/toggle.hpp"
#include "blusys/framework/ui/widgets/vu_strip/vu_strip.hpp"
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
// Eliminates the DispatchBridge boilerplate from the early interactive path.

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

// ---- passive display convenience wrappers (no action slots) ----

inline lv_obj_t *progress(lv_obj_t *parent, int32_t min = 0, int32_t max = 100,
                          int32_t initial = 0, const char *lbl = nullptr,
                          bool show_pct = false)
{
    return blusys::ui::progress_create(parent, {
        .min      = min,
        .max      = max,
        .initial  = initial,
        .label    = lbl,
        .show_pct = show_pct,
    });
}

inline lv_obj_t *level_bar(lv_obj_t *parent, int32_t min = 0, int32_t max = 100,
                           int32_t initial = 0, const char *lbl = nullptr)
{
    return blusys::ui::level_bar_create(parent, {
        .min     = min,
        .max     = max,
        .initial = initial,
        .label   = lbl,
    });
}

inline lv_obj_t *vu_strip(lv_obj_t *parent, std::uint8_t segments = 12,
                          std::uint8_t initial = 0,
                          blusys::ui::vu_orientation orient = blusys::ui::vu_orientation::vertical)
{
    return blusys::ui::vu_strip_create(parent, {
        .segment_count = segments,
        .initial       = initial,
        .orientation   = orient,
    });
}

inline lv_obj_t *icon_label(lv_obj_t *parent, const char *icon, const char *text)
{
    return blusys::ui::icon_label_create(parent, {
        .icon = icon,
        .text = text,
    });
}

inline lv_obj_t *status_badge(lv_obj_t *parent, const char *text,
                              blusys::ui::badge_level level = blusys::ui::badge_level::info)
{
    return blusys::ui::status_badge_create(parent, {
        .text  = text,
        .level = level,
    });
}

inline lv_obj_t *key_value(lv_obj_t *parent, const char *key, const char *value)
{
    return blusys::ui::key_value_create(parent, {
        .key   = key,
        .value = value,
    });
}

inline lv_obj_t *card(lv_obj_t *parent, const char *card_title = nullptr, int padding = -1)
{
    return blusys::ui::card_create(parent, {
        .title   = card_title,
        .padding = padding,
    });
}

inline lv_obj_t *gauge(lv_obj_t *parent, int32_t min = 0, int32_t max = 100,
                       int32_t initial = 0, const char *unit = nullptr)
{
    return blusys::ui::gauge_create(parent, {
        .min     = min,
        .max     = max,
        .initial = initial,
        .unit    = unit,
    });
}

inline lv_obj_t *chart(lv_obj_t *parent,
                       blusys::ui::chart_type type = blusys::ui::chart_type::line,
                       int32_t point_count = 20,
                       int32_t y_min = 0, int32_t y_max = 100)
{
    return blusys::ui::chart_create(parent, {
        .type        = type,
        .point_count = point_count,
        .y_min       = y_min,
        .y_max       = y_max,
    });
}

// ---- action-bound interactive wrappers ----

// Action-bound list: calls maker(selected_index) to produce an action.
template <typename Action>
lv_obj_t *list(lv_obj_t *parent,
               const blusys::ui::list_item *items, int32_t item_count,
               Action (*maker)(std::int32_t index),
               app_ctx *ctx,
               bool disabled = false)
{
    struct list_data {
        Action (*maker)(std::int32_t);
        app_ctx *ctx;
    };
    static_assert(sizeof(list_data) <= detail::kActionStorageSize,
                  "List maker data too large for slot storage");

    list_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for list");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;

    lv_obj_t *widget = blusys::ui::list_create(parent, {
        .items      = items,
        .item_count = item_count,
        .on_select  = +[](std::int32_t index, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const list_data *>(s->action_storage);
            auto action = d.maker(index);
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

// Action-bound dropdown: calls maker(selected_index) to produce an action.
template <typename Action>
lv_obj_t *dropdown(lv_obj_t *parent,
                   const char * const *options, int32_t option_count,
                   int32_t initial,
                   Action (*maker)(std::int32_t index),
                   app_ctx *ctx,
                   bool disabled = false)
{
    struct dd_data {
        Action (*maker)(std::int32_t);
        app_ctx *ctx;
    };
    static_assert(sizeof(dd_data) <= detail::kActionStorageSize,
                  "Dropdown maker data too large for slot storage");

    dd_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for dropdown");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;

    lv_obj_t *widget = blusys::ui::dropdown_create(parent, {
        .options      = options,
        .option_count = option_count,
        .initial      = initial,
        .on_select    = +[](std::int32_t index, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const dd_data *>(s->action_storage);
            auto action = d.maker(index);
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

// Action-bound tabs: calls maker(tab_index) to produce an action.
template <typename Action>
lv_obj_t *tabs(lv_obj_t *parent,
               const blusys::ui::tab_item *items, int32_t item_count,
               int32_t initial,
               Action (*maker)(std::int32_t index),
               app_ctx *ctx)
{
    struct tabs_data {
        Action (*maker)(std::int32_t);
        app_ctx *ctx;
    };
    static_assert(sizeof(tabs_data) <= detail::kActionStorageSize,
                  "Tabs maker data too large for slot storage");

    tabs_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for tabs");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;

    lv_obj_t *widget = blusys::ui::tabs_create(parent, {
        .items      = items,
        .item_count = item_count,
        .initial    = initial,
        .on_change  = +[](std::int32_t index, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const tabs_data *>(s->action_storage);
            auto action = d.maker(index);
            d.ctx->dispatch(action);
        },
        .user_data = slot,
    });
    if (widget != nullptr) {
        detail::attach_slot_cleanup(widget, slot);
    } else {
        detail::release_action_slot(slot);
    }
    return widget;
}

// Action-bound knob: calls maker(new_value) to produce an action.
template <typename Action>
lv_obj_t *knob(lv_obj_t *parent,
               std::int32_t min, std::int32_t max, std::int32_t initial,
               Action (*maker)(std::int32_t value),
               app_ctx *ctx,
               bool disabled = false)
{
    struct knob_data {
        Action (*maker)(std::int32_t);
        app_ctx *ctx;
    };
    static_assert(sizeof(knob_data) <= detail::kActionStorageSize,
                  "Knob maker data too large for slot storage");

    knob_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for knob");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;

    lv_obj_t *widget = blusys::ui::knob_create(parent, {
        .min       = min,
        .max       = max,
        .initial   = initial,
        .on_change = +[](std::int32_t new_value, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const knob_data *>(s->action_storage);
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

// Action-bound data_table: calls maker(row_index) on row selection.
template <typename Action>
lv_obj_t *data_table(lv_obj_t *parent,
                     const blusys::ui::table_column *columns, int32_t col_count,
                     int32_t row_count,
                     Action (*maker)(std::int32_t row),
                     app_ctx *ctx)
{
    struct table_data {
        Action (*maker)(std::int32_t);
        app_ctx *ctx;
    };
    static_assert(sizeof(table_data) <= detail::kActionStorageSize,
                  "DataTable maker data too large for slot storage");

    table_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for data_table");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;

    lv_obj_t *widget = blusys::ui::data_table_create(parent, {
        .columns       = columns,
        .col_count     = col_count,
        .row_count     = row_count,
        .on_row_select = +[](std::int32_t row, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const table_data *>(s->action_storage);
            auto action = d.maker(row);
            d.ctx->dispatch(action);
        },
        .user_data = slot,
    });
    if (widget != nullptr) {
        detail::attach_slot_cleanup(widget, slot);
    } else {
        detail::release_action_slot(slot);
    }
    return widget;
}

// Action-bound input_field: calls maker(text) on submit.
template <typename Action>
lv_obj_t *input_field(lv_obj_t *parent,
                      const char *placeholder,
                      Action (*maker)(const char *text),
                      app_ctx *ctx,
                      const char *initial = nullptr,
                      bool disabled = false)
{
    struct input_data {
        Action (*maker)(const char *);
        app_ctx *ctx;
    };
    static_assert(sizeof(input_data) <= detail::kActionStorageSize,
                  "InputField maker data too large for slot storage");

    input_data data{maker, ctx};
    auto *slot = detail::make_action_slot(data, ctx);
    if (slot == nullptr) {
        BLUSYS_LOGE("blusys_app", "action slot exhausted for input_field");
        return nullptr;
    }

    slot->dispatch_fn = nullptr;

    lv_obj_t *widget = blusys::ui::input_field_create(parent, {
        .placeholder = placeholder,
        .initial     = initial,
        .on_submit   = +[](const char *text, void *user_data) {
            auto *s = static_cast<detail::action_dispatch_slot *>(user_data);
            const auto &d = *reinterpret_cast<const input_data *>(s->action_storage);
            auto action = d.maker(text);
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

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
