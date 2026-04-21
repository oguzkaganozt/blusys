# Blusys Platform

Blusys is an internal product platform on top of ESP-IDF v5.5.4 for ESP32, ESP32-C3, and ESP32-S3 targets.

## Pick a product shape first

New products start from `blusys.project.yml` and `blusys create`. The manifest names `schema`, `interface`, `capabilities`, `profile`, and `policies`.

- [Product shape](product-shape.md) — `interactive` / `headless`, `--with`, `profile`, `--policy`, and how reference examples map to that model

At the terminal: **`blusys create --list`** prints interfaces, starter presets, capabilities, profiles, and policies.

## Guided quickstarts (after you know the shape)

<div class="grid cards" markdown>

-   **Interactive App**

    ---

    Manifest-first display product: `blusys.project.yml`, reducer model, stock widgets, and the **interactive** interface walkthrough.

    [:octicons-arrow-right-24: Interactive Quickstart](quickstart-interactive.md)

-   **Headless App**

    ---

    Manifest-first headless product: `blusys.project.yml`, the same runtime and reducer, and the **headless** walkthrough.

    [:octicons-arrow-right-24: Headless Quickstart](quickstart-headless.md)

</div>

## Cold onboarding proof

The exact quickstart walkthrough is smoke-tested in `scripts/scaffold-smoke.sh`. It clones the repo into a temp directory, creates a default starter, then runs the headless and interactive quickstarts with host builds.

## Platform Layers

| Layer | What it provides | When to use |
|-------|-----------------|-------------|
| [App](../app/index.md) | Product-facing API: reducer model, views, capabilities, profiles | **Start here** for new products |
| [Services](../services/index.md) | Runtime services: WiFi, HTTP, MQTT, OTA, storage, display | When you need direct service access beyond capabilities |
| [HAL + Drivers](../hal/index.md) | Hardware abstraction: GPIO, UART, SPI, I2C, sensors, actuators | When you need direct hardware control |
| [Internals](../internals/index.md) | Architecture, guidelines, target matrix, testing | When contributing to the platform itself |

## Product Families

The platform is optimized for these recurring product types:

- **Interactive consumer devices** — smart lights, dashboards, timers, speakers
- **Interactive local UI products** — home panels, operator screens, compact controllers
- **Headless connected appliances** — sensor nodes, infrastructure devices, appliance controllers
- **Industrial telemetry and control** — fleet devices, energy monitors, production line systems

## Requirements

- ESP-IDF v5.5+ (auto-detected by `blusys` CLI)
- Supported targets: ESP32, ESP32-C3, ESP32-S3
