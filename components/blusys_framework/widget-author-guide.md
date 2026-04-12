# Widget Author Guide

This guide is the contract for writing widgets in `blusys_framework/ui/widgets/`. It is held by code review discipline, not framework machinery — every reviewer is expected to apply it.

The reference implementation is `bu_button` (`include/blusys/framework/ui/widgets/button/button.hpp`, `src/ui/widgets/button/button.cpp`). When in doubt, copy that file pair.

For the architectural background see the [Framework guide](../../docs/guides/framework.md).

## Two categories of widgets

The widget kit has two categories. They live in different folders and follow slightly different rules.

| Category            | Folder                                | Has callbacks? | Has setters?     | Examples                                       |
|---------------------|---------------------------------------|----------------|------------------|------------------------------------------------|
| Layout primitives   | `ui/primitives/<name>.{hpp,cpp}`      | no             | rare             | `screen`, `row`, `col`, `label`, `divider`, `icon_label`, `status_badge`, `key_value` |
| Interactive widgets | `ui/widgets/<name>/<name>.{hpp,cpp}`  | yes            | yes (state-only) | `button`, `toggle`, `slider`, `modal`, `list`, `tabs`, `dropdown`, `input_field`, `knob` |
| Display widgets     | `ui/widgets/<name>/<name>.{hpp,cpp}`  | no             | yes (value only) | `progress`, `chart`, `gauge`, `card`, `data_table`, `level_bar`, `vu_strip` |

Layout primitives are stateless composition factories. Interactive widgets carry callbacks and standard widget state (`normal`, `focused`, `pressed`, `disabled`, optionally `checked`/`loading`/`error`).

This guide focuses on the interactive-widget rules. Layout primitives only need rules 1, 2, 5, and 6.

## The six-rule contract

Every widget in the kit obeys these rules, with implementation guidance below each.

### 1. Theme tokens are the only source of visual values

No hex literals, no magic pixel numbers, no inline font pointers. Every color, size, radius, and font comes from `blusys::ui::theme()`. Changing one token updates every widget that uses it on next redraw.

```cpp
// Good
const auto &t = theme();
lv_obj_set_style_bg_color(button, t.color_primary, 0);
lv_obj_set_style_radius(button, t.radius_button, 0);
lv_obj_set_style_pad_hor(button, t.spacing_md, 0);

// Bad
lv_obj_set_style_bg_color(button, lv_color_hex(0x2196F3), 0);   // hex literal
lv_obj_set_style_radius(button, 4, 0);                          // magic number
lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);   // inline font
```

If a widget needs a token that doesn't exist yet, add it to `theme_tokens` (append-only — never rename or remove a field) and populate the default in the example/product theme.

### 2. The config struct is the interface

Products pass a `<name>_config` struct at creation. They never see `lv_event_cb_t`, `lv_event_t*`, or any other LVGL event type. Callbacks use the **semantic types** in `blusys/framework/ui/callbacks.hpp` (`press_cb_t`, `toggle_cb_t`, `change_cb_t`, ...).

```cpp
// Good
struct toggle_config {
    bool         initial   = false;
    toggle_cb_t  on_change = nullptr;
    void        *user_data = nullptr;
    bool         disabled  = false;
};

// Bad
struct toggle_config {
    lv_event_cb_t on_change_raw;   // raw LVGL type leaks the impl
};
```

This rule is load-bearing: it is what makes a Camp 2 → Camp 3 migration invisible to product code.

### 3. Setters own state transitions

Products call `<name>_set_*` to change widget state. They never call `lv_obj_add_state()`, `lv_obj_clear_state()`, or style functions on a handle they received from `<name>_create()`.

```cpp
// Good
button_set_disabled(btn, true);

// Bad — product code skipping the setter
lv_obj_add_state(btn, LV_STATE_DISABLED);
```

Setters validate the handle (`if (button == nullptr) return;`) and then map directly to LVGL state mutations or theme-derived style updates.

### 4. Standard state set

Every interactive widget supports at least `normal`, `focused`, `pressed`, `disabled`. Optional extras per widget: `checked`, `loading`, `error`. Consistency across widgets is what makes encoder focus traversal work uniformly later.

The default LVGL state set (`LV_STATE_DEFAULT`, `LV_STATE_FOCUSED`, `LV_STATE_PRESSED`, `LV_STATE_DISABLED`, `LV_STATE_CHECKED`) covers all of these. You don't need custom states.

### 5. One component = one folder

Every interactive widget lives in its own folder with exactly one `.hpp` and one `.cpp`:

```text
include/blusys/framework/ui/widgets/button/button.hpp
src/ui/widgets/button/button.cpp
```

Widgets never cross-include each other except through their public headers. Shared draw helpers go in `ui/draw/`; shared input helpers go in `ui/input/`.

### 6. The header is the spec

Looking at `<name>.hpp` must tell you everything a product can do with the widget — config struct, factory function, setters, semantic intent. No hidden behavior in the `.cpp`. Document the visual intent and expected usage in a top-of-header comment.

## The slot pool pattern (Camp 2 widgets)

Camp 2 widgets wrap a stock LVGL widget. The stock widget has no room for extra instance data, so the kit attaches per-instance state via `lv_obj_set_user_data` pointing into a fixed-capacity slot pool.

### Anatomy

Use the shared helpers in `blusys/framework/ui/detail/fixed_slot_pool.hpp`. Slot structs must be **POD** with a `bool in_use` member (and any other fields your widget needs). The callback pointer can be named `on_press`, `on_select`, `on_change`, etc. — there is no required field name. **`detail::release_ui_slot`** zero-initializes the whole slot (`*slot = {}`), so every field returns to a safe released state.

```cpp
#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"

#ifndef BLUSYS_UI_<WIDGET>_POOL_SIZE
#define BLUSYS_UI_<WIDGET>_POOL_SIZE 32
#endif

namespace blusys::ui {
namespace {

struct <widget>_slot {
    <semantic_cb_t> on_<event>;
    void           *user_data;
    bool            in_use;
};

<widget>_slot g_<widget>_slots[BLUSYS_UI_<WIDGET>_POOL_SIZE] = {};

<widget>_slot *acquire_slot()
{
    return detail::acquire_ui_slot(g_<widget>_slots, kTag, "BLUSYS_UI_<WIDGET>_POOL_SIZE");
}

void release_slot(<widget>_slot *slot)
{
    detail::release_ui_slot(slot);
}
```

Display-only widgets that keep a small static **data** pool (no callbacks) use the same `acquire_ui_slot` / `release_ui_slot` pattern; the struct still needs `in_use` plus whatever fields you track (e.g. series pointers for `bu_chart`).

### Five rules the slot pool exists to enforce

1. **Acquire before create.** Call `acquire_slot()` *before* `lv_<stock>_create(parent)`. If the pool is exhausted, return `nullptr` from `<widget>_create` without ever creating an LVGL object. This guarantees no widget is ever shipped with a dead callback.

2. **Fail loud.** Pool exhaustion is `BLUSYS_LOGE` + `assert(false)`, not a silent drop. A product hitting this has mis-sized its pool and must raise `BLUSYS_UI_<WIDGET>_POOL_SIZE` via `target_compile_definitions`.

3. **One slot per instance.** The slot is attached to the widget via `lv_obj_set_user_data(obj, slot)`. Don't share slots across instances; don't reuse a slot for a different widget type.

4. **Release on `LV_EVENT_DELETE`.** Always wire an `LV_EVENT_DELETE` handler that calls `release_slot(slot)`. This is the only place that frees the slot back to the pool. Forgetting this causes the pool to leak monotonically until the assert fires.

5. **Skip the slot if there's no callback.** If `cfg.on_<event> == nullptr`, don't acquire a slot at all — the widget is purely visual. (`bu_button` does this.) The trade-off is that you can't add a callback later via a setter; products either pass it at create time or call `<widget>_destroy` and re-create. This is intentional: it keeps the pool sized for the active-callback population, not the total instance count.

### Slot lifecycle diagram

```text
button_create(cfg)
  ├─ if cfg.on_press → acquire_slot()
  │                    ├─ in_use=true
  │                    └─ return &slot   (or nullptr → return nullptr from create)
  ├─ lv_button_create(parent)
  ├─ apply theme styles
  ├─ if slot:
  │    ├─ slot->on_press = cfg.on_press
  │    ├─ slot->user_data = cfg.user_data
  │    ├─ lv_obj_set_user_data(btn, slot)
  │    ├─ add LV_EVENT_CLICKED → on_lvgl_clicked → slot->on_press(slot->user_data)
  │    └─ add LV_EVENT_DELETE  → on_lvgl_deleted → release_slot(slot)
  └─ return btn

LVGL deletes the obj
  └─ on_lvgl_deleted(e)
       └─ release_slot(slot)   ← full slot zero-init (released state)
```

### Pool sizing

Default is 32 per widget type. This is conservative for typical screens. Products override per build with:

```cmake
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    BLUSYS_UI_BUTTON_POOL_SIZE=64
    BLUSYS_UI_TOGGLE_POOL_SIZE=16
)
```

Sizing rule of thumb: count the maximum simultaneous live instances of that widget type across **all loaded screens**, then add a small headroom for transient overlays. Don't worry about the total RAM cost of pools — each slot is ~16 bytes; even 256 button slots is 4 KB.

## Camp 3 instance data (`bu_knob`)

Some widgets allocate a small per-instance struct with **LVGL’s allocator** (`lv_malloc` / `lv_free` in `LV_EVENT_DELETE`) instead of a global slot pool. Use this when you want unbounded instance counts without Kconfig pool sizing, at the cost of one allocation per interactive instance (still only at create time).

- **Public API stays the same** as Camp 2 — `knob_config`, `knob_create`, setters — products do not see LVGL events.
- **Lifecycle:** allocate `knob_data` when callbacks are needed; if `lv_arc_create` fails afterward, `lv_free` the struct before returning `nullptr`. On success, attach with `lv_obj_set_user_data`, register `LV_EVENT_VALUE_CHANGED` / `LV_EVENT_DELETE`, and `lv_free` the struct in the delete handler.
- **Failure:** if `lv_malloc` fails, log and return `nullptr` from `knob_create` without leaking half-built state.

`bu_knob` is the reference Camp 3 widget; older stock widgets remain Camp 2 until migrated individually.

## Variants and the `apply_<variant>` helper

Widgets with visual variants (e.g. `button_variant::primary` / `secondary` / `ghost` / `danger`) factor styling into a single helper used by both `<widget>_create` and `<widget>_set_variant`:

```cpp
void apply_variant(lv_obj_t *button, button_variant variant)
{
    const auto &t = theme();
    lv_obj_set_style_bg_color(button, background_for(variant, t), 0);
    // ...
}
```

This guarantees that the create-time and runtime-set paths produce identical visuals.

## Callbacks and the `+[]` idiom

Semantic callbacks are plain function pointers, not `std::function`. Captureless lambdas decay to function pointers via the `+[]` idiom:

```cpp
button_create(parent, {
    .label    = "Record",
    .on_press = +[](void *user_data) {
        auto *ctrl = static_cast<home_controller *>(user_data);
        ctrl->on_record();
    },
    .user_data = &g_home_controller,
});
```

Lambdas with captures are not supported (and won't compile against the function-pointer type). Pass state via `user_data`.

## Logging

Use `blusys/log.h` (`BLUSYS_LOGE/I/W/D`), not `esp_log.h` directly. Pick a tag of the form `"ui.<widget>"`:

```cpp
constexpr const char *kTag = "ui.button";
BLUSYS_LOGE(kTag, "slot pool exhausted (size=%d)", BLUSYS_UI_BUTTON_POOL_SIZE);
```

Logging from the kit should be rare — pool exhaustion, theme misconfiguration, hard-error paths. Steady-state widget code should be silent.

## Setter discipline

Setters read like this:

```cpp
void <widget>_set_<state>(lv_obj_t *handle, <type> value)
{
    if (handle == nullptr) {
        return;
    }
    // map (value) → LVGL state or theme-derived style
}
```

Rules:

- **Null-check the handle first.** Products should never pass null, but defensive null-checks are cheap and prevent crashes during shutdown.
- **Don't read state from the slot pool.** State is owned by LVGL (via `lv_obj_has_state`). The slot pool only holds the callback pair.
- **Don't allocate.** Setters never call `acquire_slot` or any allocator. They mutate existing LVGL state.
- **Don't trigger the callback.** A `set_state` should not invoke `on_change`. The callback is for user-driven state changes only.

## Widget checklist

Before sending a new widget for review, verify:

- [ ] Header (`<name>.hpp`) lives at `include/blusys/framework/ui/widgets/<name>/<name>.hpp`.
- [ ] Source (`<name>.cpp`) lives at `src/ui/widgets/<name>/<name>.cpp`.
- [ ] Header has a top-comment describing visual intent and expected usage.
- [ ] Config struct is `<name>_config` with default initializers.
- [ ] Callbacks in the config use semantic types from `blusys/framework/ui/callbacks.hpp`.
- [ ] All visual values come from `theme()`. No hex literals, no magic numbers, no inline fonts.
- [ ] State transitions go through `<name>_set_*` setters.
- [ ] Setters null-check the handle and do not allocate.
- [ ] Stock widget `.cpp` files are picked up by `file(GLOB ...)` in `cmake/blusys_framework_ui_sources.cmake` (path `src/ui/widgets/<name>/<name>.cpp`); no manual widget list edits.
- [ ] Header is included from `widgets.hpp`.
- [ ] At least one example exercises the create + a setter + a callback.
- [ ] Builds clean for esp32, esp32c3, and esp32s3 via `./blusys build examples/framework_ui_basic <target>`.
- [ ] `./blusys lint` passes.

**Camp 2 — slot pool** (most stock widgets):

- [ ] Slot pool macro `BLUSYS_UI_<NAME>_POOL_SIZE` is defined with a default of 32.
- [ ] `acquire_slot()` / `release_slot()` delegate to `detail::acquire_ui_slot` / `detail::release_ui_slot` (see `fixed_slot_pool.hpp`).
- [ ] `acquire_slot()` is called *before* `lv_<stock>_create()`.
- [ ] Pool exhaustion logs `BLUSYS_LOGE` and `assert(false)` (via the shared helper).
- [ ] `LV_EVENT_DELETE` handler calls `release_slot()`.

**Camp 3 — `lv_malloc` instance data** (e.g. `bu_knob` — see **Camp 3 instance data** above):

- [ ] No pool macro; per-widget struct allocated with `lv_malloc` when callback storage is needed.
- [ ] If `lv_<stock>_create` fails after `lv_malloc`, `lv_free` the struct before returning `nullptr`.
- [ ] `LV_EVENT_DELETE` handler `lv_free`s the struct (and clears user data if needed); no `release_slot()`.
