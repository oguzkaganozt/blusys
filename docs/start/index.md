# Blusys Platform

Blusys is an internal product platform on top of ESP-IDF v5.5.4 for ESP32, ESP32-C3, and ESP32-S3 targets.

## Quick Start

<div class="grid cards" markdown>

-   **Interactive App**

    ---

    Build a display-first product with the reducer model, stock widgets, and the `interactive controller` archetype.

    [:octicons-arrow-right-24: Interactive Quickstart](quickstart-interactive.md)

-   **Headless App**

    ---

    Build a connected device without a display using the same app runtime and reducer model.

    [:octicons-arrow-right-24: Headless Quickstart](quickstart-headless.md)

</div>

## Archetypes

- [Archetype Starters](archetypes.md) --- choose between `interactive controller`, `interactive panel`, `edge node`, and `gateway/controller`

## Platform Layers

| Layer | What it provides | When to use |
|-------|-----------------|-------------|
| [App](../app/index.md) | Product-facing API: reducer model, views, capabilities, profiles | **Start here** for new products |
| [Services](../services/index.md) | Runtime services: WiFi, HTTP, MQTT, OTA, storage, display | When you need direct service access beyond capabilities |
| [HAL + Drivers](../hal/index.md) | Hardware abstraction: GPIO, UART, SPI, I2C, sensors, actuators | When you need direct hardware control |
| [Internals](../internals/index.md) | Architecture, guidelines, testing infrastructure | When contributing to the platform itself |

## Product Families

The platform is optimized for these recurring product types:

- **Interactive consumer devices** --- smart lights, dashboards, timers, speakers
- **Interactive control surfaces** --- home panels, operator screens, compact controllers
- **Headless connected appliances** --- sensor nodes, infrastructure devices, appliance controllers
- **Industrial telemetry and control** --- fleet devices, energy monitors, production line systems

## Requirements

- ESP-IDF v5.5+ (auto-detected by `blusys` CLI)
- Supported targets: ESP32, ESP32-C3, ESP32-S3
