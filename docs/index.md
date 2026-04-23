# Blusys Platform

An internal ESP32 product platform built on ESP-IDF v5.5.4. Blusys provides a shared operating model for recurring product families, from interactive consumer devices to headless industrial nodes, so new products start from shared architecture instead of rebuilding plumbing.

The repository [README](https://github.com/oguzkaganozt/blusys/blob/main/README.md) (Product foundations) is the long-form source for install, validation expectations, and links to this site; these docs are the structured reference.

<div class="grid cards" markdown>

-   :material-rocket-launch:{ .lg .middle } **Start Here**

    ---

    Get oriented: product shape, manifest, then a guided interactive or headless run.

    [:octicons-arrow-right-24: Start (Getting started)](start/index.md)

-   :material-cube-outline:{ .lg .middle } **App**

    ---

    Build products: reducer flow, views, widgets, capabilities, and device profiles.

    [:octicons-arrow-right-24: App Overview](app/index.md)

-   :material-connection:{ .lg .middle } **Services**

    ---

    Connect and ship: network stacks, OTA, storage, display service, and more.

    [:octicons-arrow-right-24: Browse Services](services/index.md)

-   :material-chip:{ .lg .middle } **HAL + Drivers**

    ---

    Talk to hardware: buses, GPIO, and common sensors or actuators.

    [:octicons-arrow-right-24: Browse HAL](hal/index.md)

-   :material-wrench:{ .lg .middle } **Internals**

    ---

    Engineering depth: architecture, invariants, target matrix, tests, and contribution checks.

    [:octicons-arrow-right-24: Internals](internals/index.md)

</div>

## Supported Targets

| Target | Status |
|--------|--------|
| ESP32 | Supported |
| ESP32-C3 | Supported |
| ESP32-S3 | Supported |

See [Target Matrix](internals/target-matrix.md) for the full per-module support matrix.
