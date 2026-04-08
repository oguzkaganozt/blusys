# Blusys HAL

A simplified C hardware abstraction layer for ESP32 devices, built on top of ESP-IDF v5.5.4. Blusys exposes a smaller, more stable API surface than raw ESP-IDF so your application code stays clean across hardware revisions.

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } **Get Started**

    ---

    Set up your environment and run your first Blusys example in minutes.

    [:octicons-arrow-right-24: Getting Started](guides/getting-started.md)

-   :material-book-open-variant:{ .lg .middle } **Task Guides**

    ---

    Step-by-step guides for every peripheral — from GPIO to MQTT.

    [:octicons-arrow-right-24: Browse Guides](guides/index.md)

-   :material-code-braces:{ .lg .middle } **API Reference**

    ---

    Complete type definitions, function signatures, and error codes for every module.

    [:octicons-arrow-right-24: Browse API Reference](modules/index.md)

</div>

## Supported Targets

| Target | Status |
|--------|--------|
| ESP32 | Supported |
| ESP32-C3 | Supported |
| ESP32-S3 | Supported |

See [Compatibility](target-matrix.md) for the full per-module support matrix.

!!! info "Current Release"
    **v5.0.0** — 51 modules across HAL and Services, standalone `blusys` CLI, and full docs site.
    See [Roadmap](roadmap.md) for what's planned next.
