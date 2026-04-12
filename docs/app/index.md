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
- `blusys_ui_lock` or `lv_screen_load`
- raw LCD bring-up or Wi-Fi lifecycle orchestration

These are handled by the framework runtime, profiles, and capabilities.

## Sections

- [Reducer Model](reducer-model.md) --- state, actions, `update()`, dispatch lifecycle, intent map
- [App Runtime Model](app-runtime-model.md) --- intent/event queues, threading, what runs in `update()`
- [Views & Widgets](views-and-widgets.md) --- stock widgets, page helpers, custom widget contract, bounded LVGL
- [Widget Gallery](widget-gallery.md) --- stock widgets by product use case
- [Capability Composition](capability-composition.md) --- how capabilities fit together in `integration/`
- [Capabilities](capabilities.md) --- connectivity, storage
- [Profiles](profiles.md) --- host, headless, and device platform profiles
- [Validation and host loop](validation-host-loop.md) --- host smokes, scaffold checks, CI mapping (see README **Product foundations** → **Validation**)

## Example Apps

- [`examples/quickstart/interactive_controller/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/interactive_controller) --- canonical interactive controller quickstart using shell navigation and setup flow
- [`examples/reference/interactive_panel/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/interactive_panel) --- operational panel reference using the same app runtime and shell
- [`examples/quickstart/edge_node/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/edge_node) --- headless-first connected archetype
- [`examples/reference/gateway/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/gateway) --- coordinator/operator archetype with an optional local surface
- [`examples/reference/framework_device_basic/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/framework_device_basic) --- minimal interactive app for framework validation
- [`examples/reference/connected_device/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/connected_device) --- interactive product with connectivity capabilities
- [`examples/reference/connected_headless/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/connected_headless) --- headless product with connectivity capabilities
