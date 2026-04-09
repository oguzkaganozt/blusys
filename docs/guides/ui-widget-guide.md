# Widget Authoring Guide

This guide documents the six-rule contract every widget in the Blusys widget kit follows.
Read it before writing a new widget or modifying an existing one.

The full machine-readable contract is at
`components/blusys_framework/widget-author-guide.md` in the repo.
This guide covers the *why* behind the rules; the widget-author-guide.md covers the
*how* at the implementation level.

## Platform context

The widget kit is the UI layer of `blusys_framework`, the C++ tier of the platform. It
wraps LVGL stock primitives (`lv_button`, `lv_switch`, `lv_slider`, …) behind a
semantic product API. Product code never calls LVGL directly — it calls widget functions
(`bu_button_create`, `bu_slider_set_value`, …) and registers semantic callbacks
(`press_cb_t`, `change_cb_t`, …). This keeps product code decoupled from LVGL's
internal event system.

The widget kit targets **physical-control devices** — rotary encoders, directional
buttons, touchpads — running on embedded MCUs with small OLED or TFT panels. The UX
direction is clean, minimal, and hardware-tactile (think Teenage Engineering). No web-
style animations, no rich-color gradients, no per-widget brand theming.

## The six-rule contract

### Rule 1 — Theme tokens only

Widgets must style themselves exclusively from `blusys::ui::theme()` accessors. No
hard-coded colors, sizes, radii, or font pointers inside widget source files.

```cpp
// correct
lv_obj_set_style_bg_color(obj, theme().color_primary, 0);

// forbidden
lv_obj_set_style_bg_color(obj, lv_color_make(0, 120, 255), 0);
```

The theme is a single `theme_tokens` struct populated at boot by `app::ui::theme_init()`
in the product's `app/ui/theme_init.cpp`. Products tune colors, radii, spacing, and
fonts by filling in the struct; widgets pick up the new values automatically.

### Rule 2 — Config struct interface

Every widget exposes a `_config` struct for creation parameters and a set of explicit
setter functions for runtime state changes. No kitchen-sink constructor, no variadic
arguments.

```cpp
bu_button_config_t cfg = {
    .label   = "OK",
    .variant = BU_BUTTON_VARIANT_PRIMARY,
};
lv_obj_t *btn = bu_button_create(parent, &cfg);
bu_button_set_label(btn, "Cancel");     // post-creation setter
```

### Rule 3 — Setters own state transitions

If a widget has mutable state (label text, value, enabled/disabled, visibility), the
setter function owns the full LVGL state transition. Callers never touch the underlying
`lv_obj_t` for anything the widget exposes through its API.

### Rule 4 — Standard state set

Every interactive widget supports these states and must handle them correctly: normal,
focused, pressed/active, disabled. Use LVGL's `LV_STATE_*` flags — never derive state
from cached booleans.

### Rule 5 — One folder per widget

```text
components/blusys_framework/
  src/ui/widgets/<name>/
    <name>.cpp
  include/blusys/framework/ui/widgets/<name>/
    <name>.hpp
```

No shared widget implementation files. Each widget is self-contained.

### Rule 6 — Header is the spec

The widget's public header (`.hpp`) is the complete API contract. It contains:
- the `_config` struct (creation parameters)
- the `_create` function
- all setter and getter functions
- the callback typedef(s)
- doc comments explaining each field and function

---

## Widget camps

Widgets fall into two implementation camps based on how they store their callback slots.

### Camp 2 — Stock-backed widgets (fixed-capacity slot pool)

Camp 2 widgets wrap a stock LVGL primitive and store callbacks in a fixed-capacity
static pool allocated at compile time. Pool size is controlled by a KConfig symbol:

```
BLUSYS_UI_BUTTON_POOL_SIZE   (default 32)
BLUSYS_UI_TOGGLE_POOL_SIZE   (default 32)
BLUSYS_UI_SLIDER_POOL_SIZE   (default 32)
BLUSYS_UI_MODAL_POOL_SIZE    (default 8)
BLUSYS_UI_OVERLAY_POOL_SIZE  (default 8)
```

The pool follows a fail-loud discipline: attempting to create more widgets than the pool
holds fires a `BLUSYS_LOGE` + `LV_ASSERT_MSG(false, ...)` at runtime. This is
intentional — silent pool exhaustion is worse than an assertion.

```cpp
// Camp 2 slot allocation pattern
static bu_button_slot_t s_slots[CONFIG_BLUSYS_UI_BUTTON_POOL_SIZE];
static uint8_t          s_slot_count = 0;

static bu_button_slot_t *alloc_slot(void) {
    if (s_slot_count >= CONFIG_BLUSYS_UI_BUTTON_POOL_SIZE) {
        BLUSYS_LOGE(TAG, "button pool exhausted (%d)", CONFIG_BLUSYS_UI_BUTTON_POOL_SIZE);
        LV_ASSERT_MSG(false, "blusys: button pool exhausted");
        return nullptr;
    }
    return &s_slots[s_slot_count++];
}
```

The slot is retrieved from the LVGL object's user data pointer, set during `_create`.
The `LV_EVENT_DELETE` handler returns the slot to the pool by clearing it (or marking
it free by nulling the object pointer).

Current Camp 2 widgets: `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`.

### Camp 3 — Custom widgets (embedded storage)

Camp 3 widgets use `lv_obj_class_t` to embed their callback storage directly inside the
LVGL object allocation. No external pool needed. The `lv_obj_class_t` defines a custom
`user_data` size; LVGL allocates the full `sizeof(lv_obj_t) + user_data_size` in one
block, so the callback pointers are part of the object's own lifetime.

Camp 3 is recommended for complex composite widgets where the Camp 2 pool size is hard
to predict, or where the widget has many fields that would make a fixed pool entry large.

No V1 widgets use Camp 3 yet. `bu_knob` (deferred to V2) is the intended first Camp 3
widget.

---

## Semantic callbacks

Product code registers semantic callbacks — never `lv_event_cb_t`. The canonical types
are defined in `include/blusys/framework/ui/callbacks.hpp`:

```cpp
using press_cb_t      = void (*)(void *user_data);
using release_cb_t    = void (*)(void *user_data);
using long_press_cb_t = void (*)(void *user_data);
using toggle_cb_t     = void (*)(bool state, void *user_data);
using change_cb_t     = void (*)(int32_t value, void *user_data);
```

Widgets translate `LV_EVENT_CLICKED`, `LV_EVENT_VALUE_CHANGED`, etc. internally to
these callbacks. The `user_data` pointer is set at `_create` time and passed through
unchanged on every invocation.

---

## Thread safety

The widget kit follows the same thread-safety rules as `blusys_ui`:

- Never call widget functions from an ISR.
- When calling widget functions from the main task while a `blusys_ui` render task is
  running, hold `blusys_ui_lock(ui)` across the call.
- The recommended pattern (matching the interactive scaffold template) is:

```cpp
blusys_ui_lock(ui, BLUSYS_TIMEOUT_FOREVER);
app::ui::main_screen_create(&runtime);
blusys_ui_unlock(ui);

while (true) {
    blusys_ui_lock(ui, BLUSYS_TIMEOUT_FOREVER);
    runtime.step(now_ms());
    blusys_ui_unlock(ui);
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

Never hold `blusys_ui_lock` across a blocking wait (`vTaskDelay`, network/disk call,
long service operation). The render task holds the same lock; blocking the main task
while it is held will block the render task at its next `lv_timer_handler` call.

---

## V1 widget inventory

| Widget       | Camp | LVGL primitive  | Callbacks                      |
|--------------|------|-----------------|--------------------------------|
| `bu_button`  | 2    | `lv_button`     | `press_cb_t`                   |
| `bu_toggle`  | 2    | `lv_switch`     | `toggle_cb_t`                  |
| `bu_slider`  | 2    | `lv_slider`     | `change_cb_t`                  |
| `bu_modal`   | 2    | `lv_obj` (comp) | `press_cb_t` (dismiss)         |
| `bu_overlay` | 2    | `lv_obj` (comp) | `press_cb_t` (hidden)          |

Layout primitives (`screen`, `row`, `col`, `label`, `divider`) follow the same theme-
tokens and config-struct rules but have no interactive callbacks.

Deferred to V2: `bu_knob` (Camp 3, rotary input widget).

---

## Three-level validation chain

New widgets must pass all three levels before being considered done:

1. **Host harness** — `blusys host-build` then run `widget_kit_demo` (or write a
   targeted host test). Catches compilation errors, LVGL API misuse, and visual layout
   regressions quickly without hardware.

2. **QEMU** — `blusys qemu examples/<widget_example> esp32s3` (headless core only;
   LVGL cannot render in QEMU). Catches boot-sequence panics and spine integration.

3. **Real hardware** — build against a physical board with an LCD. Catches FreeRTOS
   render-task interactions, flush callback correctness, and timing behavior that host
   and QEMU cannot reproduce. See `docs/guides/hardware-smoke-tests.md`.

## See also

- `components/blusys_framework/widget-author-guide.md` — implementation-level contract
- `examples/framework_app_basic/` — end-to-end example: button → runtime → controller → slider + overlay
- `examples/framework_encoder_basic/` — encoder focus traversal with real `lv_indev_t`
- `docs/validation-report-v6.md` — hardware validation record
