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
- [Views & Widgets](views-and-widgets.md) --- stock widgets, page helpers, custom widget contract, bounded LVGL
- [Capabilities](capabilities.md) --- connectivity, storage
- [Profiles](profiles.md) --- host, headless, and device platform profiles

## Example Apps

- [`examples/quickstart/interactive_controller/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/interactive_controller) --- canonical interactive controller quickstart using shell navigation and setup flow
- [`examples/reference/interactive_panel/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/interactive_panel) --- operational panel reference using the same app runtime and shell
- [`examples/quickstart/framework_device_basic/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/framework_device_basic) --- minimal interactive app
- [`examples/quickstart/connected_device/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/connected_device) --- connected interactive product
- [`examples/quickstart/connected_headless/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/quickstart/connected_headless) --- connected headless product
