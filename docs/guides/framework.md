# Framework

`components/blusys_framework/` is the C++ tier of the platform — the
product surface that controllers and screens are written against. The V1
shape is locked and the full surface is shipping: a core spine for
controllers/routing/intents/feedback, a themed widget kit built on LVGL,
encoder focus helpers, and an end-to-end app example proving the chain.

This page is a task-oriented walkthrough. For the per-header API
reference, see the [Framework API page](../modules/framework.md). For
the locked planning docs, see the
[`platform-transition/`](https://github.com/oguzkaganozt/blusys/tree/main/platform-transition)
directory in the repo.

## When to use the framework tier

Use the framework tier when you're writing **product code**: a controller
that handles user input, a screen that composes widgets, an integration
that ties services together. If you're writing a HAL or service module,
stay in `blusys_services` — the framework only depends downward
(`framework → services → blusys`), never the reverse.

## Setting up a screen with the widget kit

The widget kit is gated by `BLUSYS_BUILD_UI`. The `framework_ui_basic`
example shows the minimal setup:

```cpp
#include "blusys/framework/ui/widgets.hpp"
#include "blusys/log.h"

extern "C" void app_main(void)
{
    lv_init();
    blusys::ui::set_theme({
        .color_primary    = lv_color_hex(0x2A62FF),
        .color_surface    = lv_color_hex(0x11141D),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        // ...
    });

    lv_obj_t *screen = blusys::ui::screen_create({});
    lv_obj_t *col = blusys::ui::col_create(screen, {
        .gap     = blusys::ui::theme().spacing_md,
        .padding = blusys::ui::theme().spacing_lg,
    });
    blusys::ui::label_create(col, { .text = "Hello", .font = blusys::ui::theme().font_title });
    blusys::ui::button_create(col, {
        .label    = "Press me",
        .on_press = +[](void *) { BLUSYS_LOGI("ui", "pressed"); },
    });
}
```

The full example exercises every widget in the V1 kit (`bu_button`,
`bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`) plus the encoder
focus helpers. See
[`examples/framework_ui_basic/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/framework_ui_basic).

## Wiring a controller into the runtime

The framework's core spine takes user input as `intent` events, runs them
through a `controller` that owns the application's response, and emits
either `route_command`s (for navigation / surface changes) or `feedback_event`s
(for the user). The `runtime` owns the event queue, route flush, feedback
bus, and tick cadence.

```cpp
class app_controller final : public blusys::framework::controller {
public:
    void handle(const blusys::framework::app_event &event) override
    {
        if (event.kind != blusys::framework::app_event_kind::intent) return;
        switch (blusys::framework::app_event_intent(event)) {
        case blusys::framework::intent::increment:
            // mutate state, emit feedback
            break;
        case blusys::framework::intent::confirm:
            submit_route(blusys::framework::route::show_overlay(1));
            break;
        default:
            break;
        }
    }
};

// In app_main:
blusys::framework::runtime  runtime;
app_controller              controller;
my_route_sink               route_sink;
my_feedback_sink            feedback_sink;

runtime.register_feedback_sink(&feedback_sink);
runtime.init(&controller, &route_sink, /*tick_period_ms=*/10);

runtime.post_intent(blusys::framework::intent::increment);
runtime.step(now_ms);
```

The full end-to-end app example —
[`examples/framework_app_basic/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/framework_app_basic)
— wires button widgets to `runtime.post_intent`, defines an `app_controller`
that mutates a slider directly and submits `route::show_overlay` for
overlay surfaces, defines a `ui_route_sink` that translates
`show_overlay` route commands into `overlay_show` calls on the actual
widget, and registers a logging `feedback_sink`. It is the canonical
reference for how the framework spine connects to the widget kit.

## Encoder focus traversal

`ui/input/encoder.hpp` provides two helpers for wiring encoder navigation
across a screen built with the widget kit:

```cpp
lv_group_t *group = blusys::ui::create_encoder_group();
blusys::ui::auto_focus_screen(screen, group);
// Bind your lv_indev_t for the encoder via lv_indev_set_group(indev, group);
```

`auto_focus_screen` walks the screen DFS pre-order, prunes hidden
subtrees (so a hidden `bu_modal` doesn't capture focus), and adds every
`LV_OBJ_FLAG_CLICKABLE` descendant to the group. The first focusable
widget gets focus.

The helpers are pure LVGL group/focus wiring. Products own their own
`lv_indev_t` for the encoder hardware (typically driven by the
`blusys_services` encoder driver).

## Authoring new widgets

The widget-author guide is the canonical contract:
[`components/blusys_framework/widget-author-guide.md`](https://github.com/oguzkaganozt/blusys/blob/main/components/blusys_framework/widget-author-guide.md).
Read it before adding a new widget. Highlights:

- **Six-rule contract:** theme tokens are the only source of visual
  values, the config struct is the interface, setters own state
  transitions, every widget supports the four standard states, one
  folder per widget, the header is the spec.
- **Slot pool pattern:** Camp 2 widgets store their callback in a
  fixed-capacity pool keyed by `BLUSYS_UI_<NAME>_POOL_SIZE` (default
  32). Pool exhaustion is `BLUSYS_LOGE` + `assert(false)` — products
  raise the macro via `target_compile_definitions` if they need more.
- **`bu_button` is the reference template** — every subsequent Camp 2
  widget is a near-mechanical copy with the stock LVGL widget swapped,
  the semantic callback type swapped, and the theme parts adjusted.

## Iterating on host without flashing

`scripts/host/` builds LVGL against SDL2 on Linux so widget work doesn't
require flashing hardware on every change. Install `libsdl2-dev` (or your
distro equivalent), then:

```sh
blusys host-build
./scripts/host/build-host/hello_lvgl
```

The host harness pins LVGL to the same upstream tag as the ESP-IDF
managed component (`v9.2.2`), so behaviour matches the embedded build.
See the
[host harness README](https://github.com/oguzkaganozt/blusys/blob/main/scripts/host/README.md)
for the full setup.

## Scaffolding a new product

`blusys create --starter <headless|interactive>` generates a product
project against the framework. The scaffold produces a four-CMakeLists
layout (top-level + `main/` + `app/` + `app/product_config.cmake`), an
`extern "C" void app_main(void)` in `main/app_main.cpp` that brings up
the framework runtime, a sample `home_controller`, and (for interactive)
a sample `main_screen` on the widget kit:

```sh
mkdir my_product && cd my_product
blusys create --starter interactive   # or --starter headless
```

The starter type drives `BLUSYS_BUILD_UI`: interactive products build
the framework UI tier and pull `lvgl/lvgl`; headless products skip the
UI tier entirely (zero `blusys::ui::*` symbols linked) but still route
through `blusys_framework/core` for controllers, intents, routing, and
feedback. Platform components are pulled by ESP-IDF's managed component
manager from `main/idf_component.yml` — never via `EXTRA_COMPONENT_DIRS`.

See [Getting Started](getting-started.md) for the full file tree and
walkthrough.

## Status

The V5 → V6 platform transition is complete and validated on real hardware
across all three targets (ESP32, ESP32-C3, ESP32-S3). The V1 widget kit
ships five Camp 2 widgets (`bu_button`, `bu_toggle`, `bu_slider`,
`bu_modal`, `bu_overlay`); `bu_knob` (Camp 3) is planned for V2. The full
validation record is in [`ROADMAP.md`](https://github.com/oguzkaganozt/blusys/blob/main/ROADMAP.md).

For the API reference, see the [Framework API page](../modules/framework.md).
For the layering rules and dependency direction, see
[Architecture](../architecture.md).
