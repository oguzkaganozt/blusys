# App

`blusys::app` is the product layer: state, actions, views, capabilities, and profiles.

## What lives where

| Product code owns | Framework owns |
|-------------------|----------------|
| `State`, `Action`, and `update(ctx, state, action)` | runtime, routing, overlays, and locks |
| screens, views, and widgets | input bridges and capability orchestration |
| app wiring in `main/` | host, device, and QEMU adapters |
| product-specific decisions | display bring-up and profile-driven setup |

## Read this order

1. [Reducer model](reducer-model.md)
2. [Views & widgets](views-and-widgets.md)
3. [Capabilities](capabilities.md) and [Capability composition](capability-composition.md)
4. [Profiles](profiles.md)
5. [App runtime model](app-runtime-model.md) and [Validation & host loop](validation-host-loop.md) when you need the operational path

## What the framework handles

You should not need `runtime.init`, `route_sink`, `feedback_sink`, raw `blusys_display_lock` orchestration, or ad hoc Wi-Fi bring-up. Use `ctx.fx()` for navigation and overlays, and let capabilities own lifecycle-heavy services.

## Example paths

| Path | Why read it |
|------|-------------|
| [`examples/reference/connectivity/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/connectivity) | Wi-Fi, HTTP, MQTT in a compact connected product |
| [`examples/reference/display/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/display) | LCD, LVGL, encoder, and UI flow |
| [`examples/reference/hal/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/hal) | GPIO, UART, I2C, SPI, ADC, PWM, and other low-level work |
| [`examples/reference/atlas/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/atlas) | full connected product composition |
| [`examples/reference/override/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/override) | product-local capability and explicit app spec |

Validation examples in `examples/validation/` are for regression and smoke testing, not first-time learning.
