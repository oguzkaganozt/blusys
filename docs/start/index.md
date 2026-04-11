# Blusys Platform

Blusys is an internal product platform on top of ESP-IDF v5.5.4 for ESP32, ESP32-C3, and ESP32-S3 targets.

## Pick a product archetype first

New products should start from one of four **canonical archetypes** — the same `core/` / `ui/` / `integration/` layout and reducer model; only defaults and starter content differ.

- [Archetype Starters](archetypes.md) — when to use each shape, example paths, and `blusys create --archetype …`
- [Connected products](connected-products.md) — edge node vs gateway, headless vs optional local UI (same model, different emphasis)
- [Interaction design](interaction-design.md) — controller vs panel: tactility vs operational density

At the terminal: `blusys create --list-archetypes` prints the same four options with short descriptions.

## Guided quickstarts (after you know the shape)

<div class="grid cards" markdown>

-   **Interactive App**

    ---

    Host-first display product: reducer model, stock widgets, and the **interactive controller** archetype walkthrough.

    [:octicons-arrow-right-24: Interactive Quickstart](quickstart-interactive.md)

-   **Headless App**

    ---

    Connected device without a display using the same runtime and reducer; aligns with **edge node**-style starters.

    [:octicons-arrow-right-24: Headless Quickstart](quickstart-headless.md)

</div>

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
- **Interactive control surfaces** — home panels, operator screens, compact controllers
- **Headless connected appliances** — sensor nodes, infrastructure devices, appliance controllers
- **Industrial telemetry and control** — fleet devices, energy monitors, production line systems

## Requirements

- ESP-IDF v5.5+ (auto-detected by `blusys` CLI)
- Supported targets: ESP32, ESP32-C3, ESP32-S3
