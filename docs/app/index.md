# App

`blusys::app` is the product API: reducer, views, capabilities, and profiles. Tiers and repo layout: [Architecture](../internals/architecture.md). Onboarding: [Getting started](../start/index.md).

## Model

| Concept | Owner | Description |
|---------|-------|-------------|
| **State** / **Actions** | App | Data and events |
| **`update(ctx, state, action)`** | App | Reducer (in-place state changes) |
| **Screens & views** | App | Stock or custom widgets |
| **Profiles** | Framework | Host, headless, device |
| **Capabilities** | Framework | Connectivity, storage, and other services |

## Read in this order

1. [Reducer model](reducer-model.md) → [App runtime model](app-runtime-model.md) → [Views & widgets](views-and-widgets.md) / [Widget gallery](widget-gallery.md) for UI.
2. [Capabilities](capabilities.md) + [Capability composition](capability-composition.md) → [Profiles](profiles.md).
3. Extending the stack: [Authoring a capability](capability-authoring.md), [Adding a capability](adding-a-capability.md), [Adding a flow](adding-a-flow.md), [Custom capabilities](custom-capabilities.md) — most products stop at step 2.
4. [Memory and budgets](memory-and-budgets.md) if you tune queues/heap; [Validation and host loop](validation-host-loop.md) and [Device, host & QEMU CLI](cli-host-qemu.md) for local/CI workflow.

## What the framework handles

You should not need `runtime.init`, `route_sink` / `feedback_sink`, raw `blusys_display_lock` orchestration, or ad hoc Wi-Fi bring-up — runtime, profiles, and capabilities own that.

**Prefer** `app_spec`, thin `app_main.cpp`, `update()` + **actions** + `view::` helpers (not raw `lv_` in product code). Use `row` / `col` / `page_create_in` with the shell ([UI layout](../internals/architecture.md#ui-layout-lvgl-flex)). For cases the canonical path cannot cover yet, keep HAL/service usage at clear boundaries.

## Example apps

- [reference/display](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/display) — display, LVGL, encoder
- [reference/connectivity](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/connectivity) — Wi-Fi, HTTP, MQTT
- [reference/hal](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/hal) — GPIO, PWM, UART, etc.
- [reference/override](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/override) — product-local capability
- [external](https://github.com/oguzkaganozt/blusys/tree/main/examples/external) — header-shape check

Scaffolded products: [Getting started](../start/index.md) quickstarts; deeper demos above.
