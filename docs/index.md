# Blusys Platform

An internal ESP32 product platform built on top of ESP-IDF v5.5.4. Blusys combines HAL modules, hardware drivers, runtime services, and an in-progress framework tier so new products can start from a shared architecture instead of rebuilding the same plumbing repeatedly.

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } **Get Started**

    ---

    Set up your environment and run your first Blusys example in minutes.

    [:octicons-arrow-right-24: Getting Started](guides/getting-started.md)

-   :material-book-open-variant:{ .lg .middle } **Task Guides**

    ---

    Step-by-step guides across HAL, drivers, services, and the platform transition.

    [:octicons-arrow-right-24: Browse Guides](guides/index.md)

-   :material-code-braces:{ .lg .middle } **API Reference**

    ---

    Type definitions, function signatures, and module references across the platform tiers.

    [:octicons-arrow-right-24: Browse API Reference](modules/index.md)

</div>

## Supported Targets

| Target | Status |
|--------|--------|
| ESP32 | Supported |
| ESP32-C3 | Supported |
| ESP32-S3 | Supported |

See [Compatibility](target-matrix.md) for the full per-module support matrix.

!!! info "Current State"
    The repo ships HAL, drivers, services, and the framework tier in full. The V1 widget kit (`bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`) plus the framework spine (`router`, `intent`, `feedback`, `controller`, `runtime`) and the encoder focus helpers are all live, with `examples/framework_app_basic/` validating the chain end-to-end. A PC + SDL2 host harness under `scripts/host/` lets the widget kit be iterated on without flashing hardware. See [`PROGRESS.md`](https://github.com/oguzkaganozt/blusys/blob/main/PROGRESS.md) for the current phase.
