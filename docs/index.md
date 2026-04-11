# Blusys Platform

An internal ESP32 product platform built on ESP-IDF v5.5.4. Blusys provides a shared operating model for recurring product families --- from interactive consumer devices to headless industrial nodes --- so new products start from shared architecture instead of rebuilding plumbing.

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } **Start Here**

    ---

    Create a new product and run it on host immediately.

    [:octicons-arrow-right-24: Interactive Quickstart](start/quickstart-interactive.md)

-   :material-cube-outline:{ .lg .middle } **App**

    ---

    The product-facing API: reducer model, views, widgets, capabilities, and profiles.

    [:octicons-arrow-right-24: App Overview](app/index.md)

-   :material-connection:{ .lg .middle } **Services**

    ---

    Runtime services: WiFi, HTTP, MQTT, OTA, storage, display, and more.

    [:octicons-arrow-right-24: Browse Services](services/index.md)

-   :material-chip:{ .lg .middle } **HAL + Drivers**

    ---

    Direct hardware access: GPIO, UART, SPI, I2C, sensors, actuators.

    [:octicons-arrow-right-24: Browse HAL](hal/index.md)

-   :material-wrench:{ .lg .middle } **Internals**

    ---

    Architecture, guidelines, target matrix, testing notes, and engineering references.

    [:octicons-arrow-right-24: Internals](internals/index.md)

</div>

## Supported Targets

| Target | Status |
|--------|--------|
| ESP32 | Supported |
| ESP32-C3 | Supported |
| ESP32-S3 | Supported |

See [Target Matrix](internals/target-matrix.md) for the full per-module support matrix.
