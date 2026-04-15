# App

The `blusys::app` namespace is the recommended product-facing API for building applications on the Blusys platform.

## Product Model

Every Blusys app is built around these concepts:

| Concept | Owner | Description |
|---------|-------|-------------|
| **State** | App | Your product's data model |
| **Actions** | App | Events that trigger state changes |
| **`update(ctx, state, action)`** | App | Reducer function that mutates state in place |
| **Screens & Views** | App | UI built with stock widgets or custom widgets |
| **Profiles** | Framework | Host, headless, or device hardware configuration |
| **Capabilities** | Framework | Connectivity, storage, and operational flows |

## What the framework owns

Normal product code should not need to touch:

- `runtime.init` or low-level framework boot
- `route_sink` or `feedback_sink`
- `blusys_display_lock` or `lv_screen_load`
- raw LCD bring-up or Wi-Fi lifecycle orchestration

These are handled by the framework runtime, profiles, and capabilities.

## Golden path and escape hatches

The recommended product path is `blusys::app`: `app_spec`, `update(ctx, state, action)`, `main/{core,ui,integration}/`, capabilities for connected behavior, and UI changes through **actions** and `blusys::app::view::` helpers — not raw LVGL or ad hoc service wiring.

**Avoid by default:**

1. **Raw LVGL in product screens** outside custom widgets or an explicit custom view scope — bypasses routing and locks. Use actions and view helpers.
2. **Direct connectivity orchestration** in `main/ui/` or scattered across product files — use capabilities composed in `integration/`.
3. **Large domain logic in `integration/`** — that folder is wiring (profile, capability list, `map_event`, bridges). Heavy state machines belong in `core/`.

**Layout:** prefer `blusys::row`, `blusys::col`, and `blusys::page_create_in` inside the shell so LVGL flex and scroll stay consistent. Details in [Architecture — UI layout](../internals/architecture.md#ui-layout-lvgl-flex).

**Escape hatches:** HAL and services layers remain supported for advanced cases the canonical path cannot yet express — isolate that code behind clear boundaries. See [Architecture](../internals/architecture.md) and [Guidelines](../internals/guidelines.md).

## Sections

- [Reducer Model](reducer-model.md) --- state, actions, `update()`, dispatch lifecycle, intent map
- [App Runtime Model](app-runtime-model.md) --- intent/event queues, threading, what runs in `update()`
- [Views & Widgets](views-and-widgets.md) --- stock widgets, page helpers, custom widget contract, bounded LVGL
- [Widget Gallery](widget-gallery.md) --- stock widgets by product use case
- [Capability Composition](capability-composition.md) --- how capabilities fit together in `integration/`
- [Capabilities](capabilities.md) --- connectivity, storage
- [Authoring a capability](capability-authoring.md) --- contract, reserved event IDs, reference implementation
- [Profiles](profiles.md) --- host, headless, and device platform profiles
- [Validation and host loop](validation-host-loop.md) --- host smokes, scaffold checks, CI mapping (see README **Product foundations** → **Validation**)
- [Device, host & QEMU CLI](cli-host-qemu.md) --- `blusys build` targets (`host`, `qemu*`), `blusys qemu` modes (`--graphics` / `--serial-only`)

## Example Apps

- [`examples/quickstart/handheld/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/handheld) --- canonical handheld interactive quickstart using shell navigation and setup flow
- [`examples/quickstart/headless/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/headless) --- headless-first connected reference (`interface: headless` in `inventory.yml`)
- [`examples/reference/display/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/display) --- display + LVGL + encoder scenarios (menuconfig)
- [`examples/reference/connectivity/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/connectivity) --- Wi-Fi / HTTP / MQTT client scenarios (menuconfig)
- [`examples/reference/hal/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/hal) --- HAL scenarios: GPIO, PWM, button, timer, NVS, ADC, SPI, I2C, UART (menuconfig)

Use **handheld** and **headless** quickstarts as supported product-shaped starting points.
