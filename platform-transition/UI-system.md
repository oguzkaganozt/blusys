# UI System

This document defines the Blusys platform's UI architecture normatively. For the broader platform architecture see [`application-layer-architecture.md`](application-layer-architecture.md). For the language policy see [`CPP-transition.md`](CPP-transition.md). For the decision log see [`DECISIONS.md`](DECISIONS.md).

## Position

UI is the platform's **optional subsystem**. Not every product has a display or interaction surface. Headless products build `blusys_framework/core` without `blusys_framework/ui`, and never include this subsystem.

This document is normative for **interactive products** (products with a display and some input method).

Stack:

```text
blusys                          (HAL + drivers, C: gpio, spi, lcd, button, encoder)
  → blusys_services/display/ui  (LVGL runtime, C)
  → blusys_framework/ui         (widget kit, C++)
  → app/ui                      (product screens, C++)
```

## LVGL as engine

LVGL is the rendering, layout, input, and event engine. The platform **leverages LVGL fully** — it does not replace it. Rolling a custom rendering engine would burn months on solved problems and is not part of the platform's mission.

V1 reduces LVGL exposure in normal product code, but does not attempt to hide LVGL completely. The platform hides raw event plumbing and repeated styling behind widgets and helpers; it still allows lightweight LVGL-native handles and value types where that keeps the framework simple and honest.

### What we keep from LVGL

| Layer | Keep | Why |
|---|---|---|
| Rendering | `lv_draw_rect`, `lv_draw_label`, `lv_draw_line`, `lv_draw_arc`, framebuffer, dirty-region tracking | Rendering core; not replaceable affordably |
| Layout | `lv_obj_t` flex and grid containers | Visual-neutral; just positions children |
| Events | `lv_event_t` pipeline (press, release, focus, key, value_changed, draw) | Complete event system |
| State | `lv_obj_state_t` (normal, focused, pressed, disabled, checked) | Plugs into our intent layer cleanly |
| Animation | `lv_anim_t` tween engine | Sufficient for state-feedback animations |
| Input | `lv_indev` abstraction (touch, encoder, keypad, button) | Critical for multi-input products |
| Memory | Parent-child widget ownership, auto-cleanup | Widget lifetime is LVGL's job |

### What we drop from LVGL

- `lv_button_create`, `lv_slider_create`, `lv_switch_create`, `lv_checkbox_create`, and the rest of the default widget set — normal product screen code should not call these directly. Product-specific widgets and low-level helpers may still use raw LVGL internally when needed.
- LVGL themes (`lv_theme_t`, `lv_theme_default_init`) — these are designed to restyle default widgets and are irrelevant when the platform provides its own widget kit.
- SquareLine Studio and any visual-design tool that assumes stock widgets — not used.

## Widget architecture

The widget kit has two component categories:

### Layout primitives

Composition factories over `lv_obj_create`. No new widget classes. These position and style base LVGL objects using tokens from the theme struct.

Examples: `bu_screen`, `bu_row`, `bu_col`, `bu_stack`, `bu_spacer`, `bu_divider`, `bu_label`, `bu_text_value`.

Typical implementation is ~20–40 lines per primitive:

```cpp
// blusys_framework/ui/primitives/row.cpp
namespace blusys::ui {
    lv_obj_t* row_create(lv_obj_t* parent, const row_config& cfg) {
        lv_obj_t* obj = lv_obj_create(parent);
        const auto& t = theme();
        lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(obj, cfg.gap ? cfg.gap : t.spacing_md, 0);
        lv_obj_set_style_pad_all(obj, cfg.padding, 0);
        lv_obj_set_style_bg_opa(obj, cfg.transparent ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        return obj;
    }
}
```

### Interactive widgets

Each interactive widget lives in its own folder and is implemented as either:

- **Camp 2 (stock-LVGL-backed):** the widget wraps a stock LVGL widget (`lv_button`, `lv_slider`, `lv_switch`, etc.), applies theme styles, and exposes the platform's semantic API. ~80–120 lines per widget.
- **Camp 3 (custom `lv_obj_class_t` subclass):** the widget registers a new LVGL widget class with its own draw callback, event handling, and state management. Used only when stock LVGL cannot produce the required visual (non-rectangular shapes, custom gradients, specialty renderings). ~150–300 lines per widget.

The product-facing header is **identical** in both cases. Migration between Camp 2 and Camp 3 is per-widget and invisible to product code.

V1 interactive widget list:
- `bu_button` (Camp 2 default)
- `bu_slider` (Camp 2 default)
- `bu_toggle` (Camp 2 default)
- `bu_modal` (Camp 2 default)
- `bu_overlay` (Camp 2 default — transient feedback surface)
- `bu_knob` (Camp 3, only if a pilot product needs it; otherwise deferred to V2)

V2 and beyond: `bu_meter`, `bu_waveform`, `bu_spectrum`, `bu_dial`, and other specialty renderings are canvas-based or Camp 3 custom classes. Add when a product actually needs them.

## Semantic callback types

Product code never sees LVGL's `lv_event_cb_t` or `lv_event_t*`. The framework defines a small set of semantic callback types in `blusys::ui` that express **what happened**, not **how LVGL reports it**. Each takes a `void* user_data` parameter so products can pass their controller pointer without capturing state in a lambda.

```cpp
// blusys_framework/ui/callbacks.hpp
namespace blusys::ui {

using press_cb_t    = void (*)(void* user_data);
using release_cb_t  = void (*)(void* user_data);
using click_cb_t    = void (*)(void* user_data);
using long_press_cb_t = void (*)(void* user_data);

using change_cb_t   = void (*)(int32_t new_value, void* user_data);
using toggle_cb_t   = void (*)(bool new_state, void* user_data);

using focus_cb_t    = void (*)(void* user_data);
using blur_cb_t     = void (*)(void* user_data);

using confirm_cb_t  = void (*)(void* user_data);
using cancel_cb_t   = void (*)(void* user_data);

}  // namespace blusys::ui
```

These are plain function-pointer types. They accept captureless lambdas via the `+[]` idiom (which decays a captureless lambda to a function pointer). Captures are not supported — pass state through `user_data`.

Widgets store the `(callback, user_data)` pair internally and invoke the callback when the corresponding LVGL event fires. Product code sees only the semantic callback:

```cpp
// Product code — no LVGL types anywhere
button_create(parent, {
    .label     = "Record",
    .variant   = button_variant::primary,
    .on_press  = +[](void* user_data) {
        auto* ctrl = static_cast<home_controller*>(user_data);
        ctrl->on_record();
    },
    .user_data = &g_home_controller,
});
```

### How widgets store the callback internally

**Camp 3 widgets (custom `lv_obj_class_t` subclass):** the callback and user_data are embedded directly in the widget's extended instance data (via `lv_obj_class_t::instance_size`). No allocation, no pool.

**Camp 2 widgets (stock-backed):** the stock LVGL widget has no room for extra instance data. The widget kit uses a **fixed-capacity slot pool** per widget type, sized at compile time. `button_create` acquires a slot from the pool and attaches it via `lv_obj_set_user_data`. An `LV_EVENT_DELETE` handler releases the slot when LVGL destroys the widget.

Each Camp 2 widget defines its own pool size via a preprocessor macro — `BLUSYS_UI_BUTTON_POOL_SIZE`, `BLUSYS_UI_SLIDER_POOL_SIZE`, and so on — with a conservative default (32). Products that genuinely need more instances can override the macro in product build flags (for example with `target_compile_definitions(...)` in a product-level `CMakeLists.txt`), but this is an advanced escape hatch, not the expected day-one workflow.

**Pool exhaustion is a hard error, not a silent drop.** If `acquire_slot` returns nullptr, the widget's `*_create` function logs an error, asserts (fires in debug builds), and returns `nullptr` *before* any LVGL object is created. A product that hits this has mis-sized its pool and must raise the macro — it will never ship a widget with a dead callback.

The choice between Camp 2 (pool) and Camp 3 (embedded) is invisible to product code. Both respect the fixed-capacity allocation policy from [`CPP-transition.md`](CPP-transition.md).

## Component contract

Every widget in `blusys_framework/ui/widgets/` obeys these six rules:

### 1. Theme tokens are the only source of visual values

No hex literals, no magic pixel numbers, no inline font pointers inside widget code. Every color, size, radius, and font comes from `blusys::ui::theme()` or an explicit override in the widget's config struct. Changing one token updates every widget that uses it on next redraw.

```cpp
// Good
lv_obj_set_style_bg_color(btn, theme().color_primary, 0);
lv_obj_set_style_radius(btn, theme().radius_button, 0);

// Bad
lv_obj_set_style_bg_color(btn, LV_COLOR_HEX(0x2196F3), 0);  // hex literal, forbidden
lv_obj_set_style_radius(btn, 4, 0);                          // magic number, forbidden
```

### 2. Config struct is the interface

Products pass a `<name>_config` struct at creation. They never pass `lv_event_cb_t`, `lv_event_t*`, or any other LVGL event type. Callbacks in the config use the **semantic callback types** from `blusys::ui` (`press_cb_t`, `change_cb_t`, etc.). The widget's internal code maps LVGL events to the semantic callback.

```cpp
struct button_config {
    const char*    label     = nullptr;
    button_variant variant   = button_variant::primary;
    press_cb_t     on_press  = nullptr;   // semantic type, NOT lv_event_cb_t
    void*          user_data = nullptr;
    bool           disabled  = false;
};
```

This rule is load-bearing: it is what makes Camp 2 and Camp 3 implementations interchangeable. If products saw raw LVGL types, migrating a widget from Camp 2 to Camp 3 would break callsites.

### 3. Setters own state transitions

Products call `<name>_set_*` functions to change widget state. They never call `lv_obj_add_state()`, `lv_obj_clear_state()`, or manipulate styles directly on a widget they received from a `bu_*_create()`.

```cpp
// Good
button_set_disabled(btn, true);

// Bad
lv_obj_add_state(btn, LV_STATE_DISABLED);
```

### 4. Standard state set

Every interactive widget handles at least four states: `normal`, `focused`, `pressed`, `disabled`. Optional extras per widget: `checked`, `loading`, `error`. Consistency across widgets is what makes encoder focus traversal work uniformly.

### 5. One component = one folder

Every widget lives in its own folder with exactly one `.hpp` and one `.cpp`:

```text
blusys_framework/ui/widgets/button/
    button.hpp
    button.cpp
```

Shared draw helpers live in `blusys_framework/ui/draw/`. Shared input helpers live in `blusys_framework/ui/input/`. Widgets never cross-include each other except through their public headers.

### 6. The header is the spec

Looking at `<name>.hpp` must tell you everything a product can do with the widget. No hidden behavior in the `.cpp`. Document the visual intent and expected usage in the header comment.

---

These six rules are the contract that keeps the design system consistent over time. They are held by code review discipline, not by framework machinery.

## Theme model

A single struct, populated by the product at boot.

```cpp
namespace blusys::ui {

struct theme_tokens {
    // Colors
    lv_color_t color_primary;
    lv_color_t color_surface;
    lv_color_t color_on_primary;
    lv_color_t color_accent;
    lv_color_t color_disabled;

    // Spacing (pixels)
    int spacing_sm;
    int spacing_md;
    int spacing_lg;

    // Radius (pixels)
    int radius_card;
    int radius_button;

    // Fonts
    const lv_font_t* font_body;
    const lv_font_t* font_title;
    const lv_font_t* font_mono;
};

void               set_theme(const theme_tokens& t);
const theme_tokens& theme();

}  // namespace blusys::ui
```

Product populates the struct at boot:

```cpp
// app/ui/theme_init.cpp
#include "blusys/framework/ui/theme.hpp"
#include "lvgl.h"

namespace app::ui {
void theme_init() {
    blusys::ui::set_theme({
        .color_primary    = LV_COLOR_HEX(0x2196F3),
        .color_surface    = LV_COLOR_HEX(0x1E1E1E),
        .color_on_primary = LV_COLOR_HEX(0xFFFFFF),
        .color_accent     = LV_COLOR_HEX(0xFF9800),
        .color_disabled   = LV_COLOR_HEX(0x555555),
        .spacing_sm = 8, .spacing_md = 16, .spacing_lg = 24,
        .radius_card = 8, .radius_button = 4,
        .font_body  = &lv_font_montserrat_14,
        .font_title = &lv_font_montserrat_20,
        .font_mono  = &lv_font_unscii_8,
    });
}
}
```

### Rules for the theme struct

- **Append-only.** New fields get default values so existing products keep building. Never remove or rename a field.
- **Read at draw time.** Widgets read `theme()` during their draw callback or create function. They do not cache token values.
- **Runtime switching works for free.** Calling `set_theme(new_tokens)` followed by `lv_obj_invalidate(lv_scr_act())` forces a redraw with the new tokens. Day/night mode is free if it's needed.
- **No JSON, no Python, no build-time generation, no runtime parser in V1.** If a future need for JSON emerges, a helper can parse JSON into `theme_tokens`. The widget contract does not change.

## Camp 2 and Camp 3 side-by-side

The same widget header, two implementations. **Neither exposes LVGL event types or raw stock-widget contracts to product code.** Some LVGL handle and value types remain visible intentionally in V1.

**Header (identical in both cases):**

```cpp
// blusys_framework/ui/widgets/button/button.hpp
#pragma once
#include "blusys/framework/ui/callbacks.hpp"
#include "lvgl.h"   // only because button_create returns lv_obj_t* as an opaque handle

namespace blusys::ui {

enum class button_variant : uint8_t {
    primary, secondary, ghost, danger,
};

struct button_config {
    const char*    label     = nullptr;
    button_variant variant   = button_variant::primary;
    press_cb_t     on_press  = nullptr;
    void*          user_data = nullptr;
    bool           disabled  = false;
};

lv_obj_t* button_create      (lv_obj_t* parent, const button_config& cfg);
void      button_set_label   (lv_obj_t* btn, const char* label);
void      button_set_variant (lv_obj_t* btn, button_variant v);
void      button_set_disabled(lv_obj_t* btn, bool disabled);

}  // namespace blusys::ui
```

Note: `lv_obj_t*` appears in the public header only as an opaque handle. Product code treats it as a handle and should not use it as an excuse to drop back to raw LVGL for normal widget styling or event wiring. Hiding it entirely behind a platform-specific alias is possible later, but it is not a V1 goal.

**Camp 2 implementation (stock-backed) with fixed-capacity slot pool:**

```cpp
// button.cpp — Camp 2
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"
#include <cassert>

#ifndef BLUSYS_UI_BUTTON_POOL_SIZE
#define BLUSYS_UI_BUTTON_POOL_SIZE 32
#endif

namespace blusys::ui {
namespace {

struct button_slot {
    press_cb_t on_press;
    void*      user_data;
    bool       in_use;
};

button_slot g_button_slots[BLUSYS_UI_BUTTON_POOL_SIZE] = {};

button_slot* acquire_slot() {
    for (auto& s : g_button_slots) {
        if (!s.in_use) { s.in_use = true; return &s; }
    }
    // Pool exhausted: fail loud. Product must raise BLUSYS_UI_BUTTON_POOL_SIZE.
    BLUSYS_LOGE("ui.button",
                "slot pool exhausted (size=%d) — raise BLUSYS_UI_BUTTON_POOL_SIZE",
                BLUSYS_UI_BUTTON_POOL_SIZE);
    assert(false && "bu_button slot pool exhausted");
    return nullptr;
}

void release_slot(button_slot* s) {
    if (s) { s->on_press = nullptr; s->user_data = nullptr; s->in_use = false; }
}

void on_lvgl_clicked(lv_event_t* e) {
    auto* obj  = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* slot = static_cast<button_slot*>(lv_obj_get_user_data(obj));
    if (slot && slot->on_press) {
        slot->on_press(slot->user_data);
    }
}

void on_lvgl_deleted(lv_event_t* e) {
    auto* obj  = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* slot = static_cast<button_slot*>(lv_obj_get_user_data(obj));
    release_slot(slot);
}

}  // anonymous namespace

lv_obj_t* button_create(lv_obj_t* parent, const button_config& cfg) {
    // Acquire the slot up front so we fail before creating any LVGL object.
    // This guarantees we never ship a widget with a dead callback.
    button_slot* slot = nullptr;
    if (cfg.on_press) {
        slot = acquire_slot();
        if (!slot) {
            return nullptr;   // pool exhausted — fail loud, create nothing
        }
    }

    lv_obj_t* btn = lv_button_create(parent);        // stock widget
    const auto& t = theme();

    // Styling — all from theme tokens
    lv_obj_set_style_bg_color(btn,
        cfg.variant == button_variant::primary ? t.color_primary : t.color_surface, 0);
    lv_obj_set_style_radius      (btn, t.radius_button, 0);
    lv_obj_set_style_pad_all     (btn, t.spacing_sm, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    if (cfg.label) {
        auto* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, cfg.label);
        lv_obj_set_style_text_color(lbl, t.color_on_primary, 0);
        lv_obj_set_style_text_font (lbl, t.font_body, 0);
        lv_obj_center(lbl);
    }

    // Wire the semantic callback through the slot we already acquired
    if (slot) {
        slot->on_press  = cfg.on_press;
        slot->user_data = cfg.user_data;
        lv_obj_set_user_data(btn, slot);
        lv_obj_add_event_cb(btn, on_lvgl_clicked, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(btn, on_lvgl_deleted, LV_EVENT_DELETE,  nullptr);
    }

    if (cfg.disabled) {
        lv_obj_add_state(btn, LV_STATE_DISABLED);
    }
    return btn;
}

}  // namespace blusys::ui
```

Notes on this implementation:
- The slot is acquired **before** `lv_button_create`, so a pool exhaustion never leaks a half-wired LVGL object.
- `acquire_slot` asserts and returns nullptr; `button_create` returns nullptr in turn. Products check the return value, just like they would for any other fallible create call.
- Pool size is per widget type, overridable per product via `-DBLUSYS_UI_BUTTON_POOL_SIZE=N` in product build flags. The default (32) is intentionally modest — products with more widgets must opt in explicitly, which surfaces the budget during development instead of hiding it in a platform default.
- In normal products, the defaults should be enough. Reaching for pool overrides is a sign that a product has unusually dense screens and should make that budget tradeoff explicitly.

**Camp 3 implementation (custom class) with embedded storage:**

```cpp
// button.cpp — Camp 3
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/draw/draw.hpp"

namespace blusys::ui {
namespace {

struct bu_button_t {
    lv_obj_t       obj;             // must be first — LVGL inheritance
    char           label[32];
    button_variant variant;
    press_cb_t     on_press;
    void*          user_data;
};

void bu_button_constructor(const lv_obj_class_t*, lv_obj_t* obj) {
    auto* b = reinterpret_cast<bu_button_t*>(obj);
    b->label[0]   = '\0';
    b->variant    = button_variant::primary;
    b->on_press   = nullptr;
    b->user_data  = nullptr;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

void bu_button_event(const lv_obj_class_t*, lv_event_t* e) {
    auto  code = lv_event_get_code(e);
    auto* obj  = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* b    = reinterpret_cast<bu_button_t*>(obj);

    if (code == LV_EVENT_DRAW_MAIN) {
        // Custom draw — where visual identity lives
        const auto& t = theme();
        lv_layer_t* layer = lv_event_get_layer(e);
        lv_area_t area; lv_obj_get_coords(obj, &area);

        lv_color_t fill = (b->variant == button_variant::primary)
                        ? t.color_primary : t.color_surface;
        draw::filled_rounded_rect(layer, area, fill, t.radius_button);
        draw::centered_text(layer, area, b->label, t.font_body, t.color_on_primary);
        if (lv_obj_has_state(obj, LV_STATE_FOCUSED)) {
            draw::focus_ring(layer, area, t.color_accent, t.radius_button);
        }
    } else if (code == LV_EVENT_CLICKED && b->on_press) {
        b->on_press(b->user_data);
    }
}

const lv_obj_class_t bu_button_class = {
    .base_class     = &lv_obj_class,
    .constructor_cb = bu_button_constructor,
    .event_cb       = bu_button_event,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .instance_size  = sizeof(bu_button_t),
};

}  // anonymous namespace

lv_obj_t* button_create(lv_obj_t* parent, const button_config& cfg) {
    // LVGL custom-widget lifecycle is two-step: allocate, then init.
    // create_obj returns an uninitialized instance; init_obj fires the
    // class constructor (bu_button_constructor) which sets defaults.
    // Only after init is it safe to apply caller-supplied config.
    lv_obj_t* obj = lv_obj_class_create_obj(&bu_button_class, parent);
    lv_obj_class_init_obj(obj);
    auto* b = reinterpret_cast<bu_button_t*>(obj);

    if (cfg.label) {
        // bounded copy, never allocates
        size_t n = 0;
        while (cfg.label[n] && n < sizeof(b->label) - 1) { b->label[n] = cfg.label[n]; ++n; }
        b->label[n] = '\0';
    }
    b->variant   = cfg.variant;
    b->on_press  = cfg.on_press;
    b->user_data = cfg.user_data;

    if (cfg.disabled) {
        lv_obj_add_state(obj, LV_STATE_DISABLED);
    }
    return obj;
}

}  // namespace blusys::ui
```

**Product code (identical regardless of implementation):**

```cpp
button_create(screen, {
    .label     = "Record",
    .variant   = button_variant::primary,
    .on_press  = +[](void* user_data) {
        auto* ctrl = static_cast<home_controller*>(user_data);
        ctrl->on_record();
    },
    .user_data = &g_home_controller,
});
```

The product never sees `lv_event_t*`, `lv_event_cb_t`, or any LVGL event type. The contract is the abstraction boundary, and it is clean in both implementations.

## Draw and input helpers

`blusys_framework/ui/draw/` ships helpers that wrap common LVGL draw patterns, cutting boilerplate from widget draw callbacks:

```cpp
namespace blusys::ui::draw {
    void filled_rounded_rect (lv_layer_t*, const lv_area_t&, lv_color_t, int radius);
    void stroked_rounded_rect(lv_layer_t*, const lv_area_t&, lv_color_t, int radius, int border_width);
    void centered_text       (lv_layer_t*, const lv_area_t&, const char*, const lv_font_t*, lv_color_t);
    void focus_ring          (lv_layer_t*, const lv_area_t&, lv_color_t, int radius);
}
```

`blusys_framework/ui/input/` ships helpers that simplify input device setup:

```cpp
namespace blusys::ui::input {
    lv_group_t* create_encoder_group(lv_indev_read_cb_t read_cb);
    void        auto_focus_screen   (lv_group_t* group, lv_obj_t* screen);
    // auto_focus_screen walks the screen's children and adds every focusable
    // object to the group, so product code doesn't have to call lv_group_add_obj
    // manually for each widget.
}
```

These helpers intentionally keep a little LVGL in view because they are close to the engine boundary. Ordinary screen composition should spend most of its time in platform widgets and controller callbacks, not in LVGL input plumbing.

## Directory layout

```text
components/blusys_framework/
  include/blusys/framework/ui/
    theme.hpp                    ← theme_tokens + set_theme + theme()
    widgets.hpp                  ← umbrella header
    primitives/
      screen.hpp
      row.hpp
      col.hpp
      stack.hpp
      spacer.hpp
      divider.hpp
      label.hpp
      text_value.hpp
    widgets/
      button/button.hpp
      slider/slider.hpp
      toggle/toggle.hpp
      modal/modal.hpp
      overlay/overlay.hpp
      knob/knob.hpp              ← optional, Camp 3
    draw/draw.hpp
    input/input.hpp

  src/ui/
    theme.cpp
    primitives/
      screen.cpp
      row.cpp
      col.cpp
      ...
    widgets/
      button/button.cpp
      slider/slider.cpp
      ...
    draw/draw.cpp
    input/input.cpp
```

Product-side:

```text
app/ui/
  theme_init.cpp        ← populates blusys::ui::theme_tokens
  screens/
    main_screen.cpp
    settings_screen.cpp
  widgets/              ← product-specific widgets if needed
    battery_indicator.cpp
```

## UI and controller boundary

- Screens never call services directly.
- Screens never make route decisions.
- Screens never contain business logic.
- Route decisions are owned by controllers.
- Transient feedback is handled through overlays.

Dependency direction:

```text
blusys_framework → blusys_services → blusys (HAL + drivers)
```

No reverse dependencies.

## Code style for screens

Screens are composition-focused. They use platform widgets like building blocks:

```cpp
// app/ui/screens/main.cpp
#include "blusys/framework/ui/widgets.hpp"
#include "controllers/home_controller.hpp"   // app/ is this file's own component;
                                              // its include root is ".", so paths
                                              // are relative to the app/ directory.

namespace app::ui {

void main_screen_show(home_controller* ctrl) {
    using namespace blusys::ui;
    const auto& t = theme();

    auto* screen = screen_create({});
    auto* col    = col_create(screen, { .gap = t.spacing_lg,
                                        .padding = t.spacing_md });

    label_create(col, { .text = "Blusys", .font = t.font_title });

    button_create(col, {
        .label     = "Start",
        .variant   = button_variant::primary,
        .on_press  = +[](void* user_data) {
            static_cast<home_controller*>(user_data)->on_start();
        },
        .user_data = ctrl,
    });

    lv_scr_load(screen);
}

}  // namespace app::ui
```

The screen code contains zero LVGL event types. All callbacks are captureless lambdas that take `void* user_data` and cast it to the controller pointer. The `+[]` forces the lambda to decay to a plain function pointer (required because our callback types are function pointers, not `std::function`).

Screens never write raw `lv_obj_set_style_*` calls. Repeated styling lives inside widgets. Screens compose widgets.

Direct LVGL calls at screen/bootstrap boundaries are still acceptable in V1 when they are structural rather than stylistic — for example `lv_scr_load(screen)` or a product-specific custom widget implementation. The goal is to remove routine boilerplate and inconsistency, not to pretend LVGL does not exist.

## Input, router, and feedback flow

Default flow for interactive products:

```text
hardware / transport input
  → platform input source (driver or service)
  → framework input adapter
  → intent-level event
  → controller
  → state transition
  → route command
  → screen update / feedback
```

Typical input sources in V1:
- driver-backed: `button`, `encoder`
- service-backed: `usb_hid`

Intent types:

```cpp
enum class intent : uint8_t {
    press, long_press, release,
    confirm, cancel,
    increment, decrement,
    focus_next, focus_prev,
};
```

Router command set (repeated from [`application-layer-architecture.md`](application-layer-architecture.md)):

- `set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`

Rules:
- Screens forward input as events to controllers. They do not decide routing.
- Controllers produce router commands.
- Transient feedback uses the overlay channel, not router commands.
- Headless products do not emit router commands by default.

## UX direction

Reference experience: physical-control devices where fast, tactile feedback matters (Teenage Engineering-style industrial design is a good mental model for the *feel*, not the visual specifics).

Implications:
- Physical inputs (knob, button, slider, encoder) are the primary control surface.
- Focus, selection, and value changes must be immediately readable on screen.
- Animations are for state feedback, not decoration.
- Every interaction has at least one instant feedback channel.

Feedback channel options:
- Visual (screen update, overlay, LED)
- Audio (buzzer, tone)
- Haptic (vibration motor, click feedback)

Product hardware determines which channels are available. The framework's feedback API is channel-agnostic — products register sinks for the channels they support.

## LVGL thread safety

`blusys_ui_lock` / `blusys_ui_unlock` are required whenever LVGL objects are created or mutated from a task other than the LVGL render task.

```cpp
blusys_ui_lock(ui);
button_create(screen, { .label = "OK" });
blusys_ui_unlock(ui);
```

No lock is needed when already inside the render task (e.g., inside a widget's draw or event callback).

### Critical rule

**Never hold `blusys_ui_lock` across a blocking wait.**

Forbidden under the lock:
- `vTaskDelay`
- event wait
- network wait
- disk wait
- any long-running service call

Correct pattern:

```text
take lock → apply short UI mutation → release lock → wait/work elsewhere
```

## Memory and asset notes

- Font choice, glyph range, and whether to use an icon font or bitmap assets are architectural decisions per product. The framework ships no default font; products compile their own (bitmap `.c` files generated via `lv_font_conv`) and set font pointers in `theme_tokens`.
- Animations should be lightweight — prefer property and style changes over layout rebuilds.
- Value and selection feedback must feel instant (sub-frame latency when possible).
- SRAM pressure is real on ESP32-C3. Fewer widget instances and smaller framebuffers beat clever graphics.

## Validation

UI validation has three levels:

| # | Layer | What it validates | What it does not validate | Needs hardware? |
|---|---|---|---|---|
| 1 | PC + LVGL + SDL2 | Widget, layout, theme — fastest UI iteration | ESP firmware, FreeRTOS, real LCD driver, real timing, memory pressure | No |
| 2 | ESP-IDF QEMU | Firmware boot, render task lifecycle, static UI flow | Real LCD/DMA, real timing, SRAM pressure, touch, power | No |
| 3 | Real hardware | Timing, SRAM pressure, panel, DMA, touch/encoder/knob feel | — | Yes |

Summary:

```text
PC + SDL2 → ESP-IDF QEMU → real hardware
```

For platform readiness, keep the validation output concrete but light: note first-boot success, measure input → feedback latency for the interactive reference product, and record RAM/flash deltas instead of inventing a large test matrix.

## Non-goals

- Rewriting the `blusys_ui` runtime or replacing LVGL.
- Forcing every product into the same visual style.
- Shipping a runtime theme system in V1.
- Shipping a JSON theme pipeline, Python theme tooling, or design-tool integration in V1.
- Writing screen code that contains service calls or hardware logic.
- Forcing a UI stack on headless products.

## Summary

- LVGL is the engine. Keep it, leverage it, do not fight it.
- The widget kit has two categories: layout primitives (composition factories) and interactive widgets (Camp 2 stock-backed or Camp 3 custom class).
- The product-facing API is the same regardless of implementation. Migration between Camp 2 and Camp 3 is per-widget and invisible to product code.
- Every widget follows the six-rule component contract.
- Theme is one struct, populated at boot. No JSON, no pipeline, no tooling.
- V1 reduces LVGL exposure in normal product code rather than eliminating it entirely.
- Route decisions are owned by controllers. Screens compose widgets.
- UI is the platform's optional subsystem. Headless products build `blusys_framework/core` without this kit.

The widget kit is what makes Blusys a platform rather than a library. Its consistency is the product of convention and discipline — the six-rule contract, the theme struct, and the folder structure — not framework machinery.
