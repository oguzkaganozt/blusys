# Getting started

Targets **ESP32 / ESP32-C3 / ESP32-S3** with **ESP-IDF 5.5+**. [README](https://github.com/oguzkaganozt/blusys/blob/main/README.md): install, validation, and CLI overview.

1. **Shape** — [Product shape](product-shape.md): `blusys create`, `blusys.project.yml`, `interface`, `capabilities`, `profile`, `policies`. Run **`blusys create --list`** for presets.
2. **Quickstart** — pick **interactive** (UI + `main/ui/`) or **headless** (terminal) below; then [App](../app/index.md) for the reducer and capabilities.
3. **Optional:** `scripts/scaffold-smoke.sh` clones a temp tree and runs both quickstart host builds (CI-style smoke).

<div class="grid cards" markdown>

-   **Interactive App**

    ---

    `blusys create --interface interactive …` — SDL host window, `main/ui/`.

    [:octicons-arrow-right-24: Interactive Quickstart](quickstart-interactive.md)

-   **Headless App**

    ---

    `blusys create --interface headless …` — same runtime, no `ui/` by default.

    [:octicons-arrow-right-24: Headless Quickstart](quickstart-headless.md)

</div>

## Where things live

| Layer | Role |
|-------|------|
| [App](../app/index.md) | Reducer, views, capabilities, profiles (product code starts here) |
| [Services](../services/index.md) | C services when you go below capabilities |
| [HAL + drivers](../hal/index.md) | Direct hardware |
| [Internals](../internals/index.md) | Architecture, layer checks, target matrix, contributing |

## Product types (examples)

Interactive consumer / panels; headless connected appliances; industrial telemetry — same platform, different `interface` and capability sets.

## Requirements

ESP-IDF v5.5+; supported chips as above.
