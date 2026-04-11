# Blusys

**Internal ESP32 product platform** on [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) **v5.5+**. One dependency stack, one CLI, three supported silicon targets — **ESP32**, **ESP32-C3**, and **ESP32-S3** — and a product-facing API in `blusys::app` built around reducers, screens, and optional capabilities.

---

## Why this repo exists

Blusys is not a generic “better ESP-IDF.” It is a shared **operating model** for recurring product families: same app shape (`core/` · `ui/` · `integration/`), same `update(ctx, state, action)` loop, host-first iteration where it helps, and escape hatches to HAL and services when you need them.

---

## Quick start

```sh
# One-time install
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
~/.blusys/install.sh

# New project (default archetype: interactive controller)
mkdir ~/my_product && cd ~/my_product
blusys create --archetype interactive-controller

# Other archetypes — see: blusys create --list-archetypes

# Build, flash, and monitor (from the project directory)
blusys run /dev/ttyACM0 esp32s3
```

ESP-IDF **v5.5+** is required; the CLI discovers it. For the full walkthrough — archetypes, connected products, host loop — start at **[Getting started](docs/start/index.md)**.

---

## Architecture

Three ESP-IDF components, strict one-way dependencies:

```
                    ┌─────────────────────────────────────┐
                    │  Application (your product code)    │
                    └──────────────────┬──────────────────┘
                                       │
         ┌─────────────────────────────▼────────────────────────────┐
         │  Framework    components/blusys_framework/   (C++)       │
         │               blusys::app, routing, widgets, capabilities  │
         └─────────────────────────────┬────────────────────────────┘
                                       │
         ┌─────────────────────────────▼────────────────────────────┐
         │  Services     components/blusys_services/    (C)         │
         │               Wi-Fi, UI runtime, protocols, system         │
         └─────────────────────────────┬────────────────────────────┘
                                       │
         ┌─────────────────────────────▼────────────────────────────┐
         │  HAL + drivers  components/blusys/           (C)         │
         │                 peripherals, displays, sensors, …        │
         └─────────────────────────────┬────────────────────────────┘
                                       │
                                       ▼
                                  ESP-IDF → hardware
```

**Rule:** `blusys_framework` → `blusys_services` → `blusys`. No reverse edges; layering is checked with `blusys lint`.

| Layer | Role | Doc entry |
|-------|------|-----------|
| **Framework** | Product API, views, capabilities, host/device profiles | [App](docs/app/index.md) |
| **Services** | Runtime modules (connectivity, storage, UI service, …) | [Services](docs/services/index.md) |
| **HAL + drivers** | Registers, buses, and higher-level driver helpers | [HAL](docs/hal/index.md) |

Module and example indices live in **`inventory.yml`** (CI and classification source of truth).

---

## Usage snippets

**Product code (recommended):**

```cpp
#include "blusys/app/app.hpp"
```

**HAL and services (direct):**

```c
#include "blusys/blusys.h"
#include "blusys/blusys_services.h"
```

**CMake `REQUIRES`:**

```cmake
idf_component_register(SRCS "main.c"   REQUIRES blusys)
idf_component_register(SRCS "main.c"   REQUIRES blusys_services)
idf_component_register(SRCS "main.cpp" REQUIRES blusys_framework)
```

Widget authoring: **[widget-author-guide.md](components/blusys_framework/widget-author-guide.md)**.

---

## Host harness

LVGL on **SDL2** (Linux) for fast UI iteration without flashing every change. Setup: **[scripts/host/README.md](scripts/host/README.md)**.

```sh
sudo apt install libsdl2-dev   # or your distro’s SDL2 dev package
blusys host-build
./scripts/host/build-host/hello_lvgl
```

---

## Examples

| Path | Purpose |
|------|---------|
| `examples/quickstart/` | Archetype-aligned starters (`blusys create` shapes), PR CI |
| `examples/reference/` | Deeper demos and per-area examples, nightly CI |
| `examples/validation/` | Internal smoke and stress, nightly CI |

Details and flags: **`inventory.yml`**.

---

## Documentation

**[Published site](https://oguzkaganozt.github.io/blusys/)** — Start → App → Services → HAL → Internals.

Local preview:

```sh
pip install -r requirements-docs.txt
mkdocs serve
```

---

## Project status

Current **package version** is **6.1.0** (`BLUSYS_VERSION_STRING` in [`components/blusys/include/blusys/version.h`](components/blusys/include/blusys/version.h); also `blusys version`). Product requirements: **[PRD.md](PRD.md)**. Contributor conventions: **[CLAUDE.md](CLAUDE.md)**.

---

## License

See **[LICENSE](LICENSE)**.
