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
    The repo currently ships HAL, drivers, and services today, with the framework tier being introduced during the platform transition.
