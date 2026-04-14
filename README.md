# Blusys

**Internal ESP32 product platform** on [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) **v5.5+**. One dependency stack, one CLI, three supported silicon targets — **ESP32**, **ESP32-C3**, and **ESP32-S3** — and a product-facing API in `blusys::app` built around reducers, screens, and optional capabilities.

---

## Why this repo exists

Blusys is not a generic “better ESP-IDF.” It is a shared **operating model** for recurring product families: same app shape (`core/` · `ui/` · `platform/`), same `update(ctx, state, action)` loop, host-first iteration where it helps, and escape hatches to HAL and services when you need them.

### Product foundations

These are the grounding constraints for the platform.

**Mission.** Blusys is the **internal OS** for recurring product shapes — not a generic ESP32 framework. Optimize for what we ship: shared lifecycle, interaction grammar, and runtime orchestration. HAL, services, `blusys/drivers/display`, and low-level primitives stay available as **escape hatches**, not the default product path.

**B2C / B2B north stars.** The platform supports both consumer (B2C) and industrial / business (B2B) products on **one** shared model. For B2C, aim for interaction and character reminiscent of **[Teenage Engineering](https://teenage.engineering/)** — distinctive, tactile, concise, memorable. For B2B, aim for operational clarity reminiscent of **[Samsara](https://www.samsara.com/)** — readable, dependable, fluent, connected. These are design and outcome targets, not separate framework runtimes.

**Locked decisions.** Product API: **`blusys::app`** (C++ on the product path). App logic: reducer **`update(ctx, state, action)`** with **in-place** state mutation. Core runtime modes: **`interactive`** and **`headless`** only. Default onboarding: **host-first** interactive; secondary path: **headless-first** hardware. **Consumer** and **industrial** — the usual **B2C** vs **B2B** split — are product **lenses**, not framework branches (see **B2C / B2B north stars** above). Public term: **`capabilities`** (not “bundles”). First canonical interactive hardware profile: **generic SPI ST7735**, building on **ESP32**, **ESP32-C3**, and **ESP32-S3**. **Code-first** hardware and capability configuration; **Kconfig** for advanced tuning. **Raw LVGL** only inside custom widgets or an explicit custom view scope; UI drives behavior through **actions** and approved framework behavior.

**Product shape** (scaffold axes): **`--interface`** (`handheld` | `surface` | `headless`), **`--with`** (framework capabilities), **`--policy`** (non-capability overlays such as `low_power`). Generated projects record the same shape in **`blusys.project.yml`**. Run **`blusys create --list`** for descriptions and dependency rules.

**Default layout** (single local **`main/`** component): **`core/`** — state, actions, reducer, product behavior; **`ui/`** — screens and widgets from state, dispatch actions (**no direct runtime-service calls**); **`platform/`** — wiring, profile, capabilities, bridges (**thin assembly**, not business logic). Mirrors the framework: `core/` ↔ `framework/app/`+`capabilities/`+`flows/`, `ui/` ↔ `framework/ui/`, `platform/` ↔ `framework/platform/` (the sole escape hatch to HAL/drivers/services).

**Split.** The **app** owns state, actions, `update`, screens/views, optional profiles and capabilities. The **framework** owns boot/shutdown, loop/tick, routing, feedback, LVGL lifecycle and locks, overlays, host/device/headless adapters, input bridges, default service orchestration, and reusable flows. Use `ctx.services()` for navigation, overlays, and filesystem handles exposed there — do not use `route_sink` directly from product code. Normal product code should not depend on `runtime.init`, `feedback_sink`, `blusys_display_lock`, `lv_screen_load`, raw LCD bring-up on the canonical path, or raw Wi-Fi orchestration on the canonical path.

**Non-goals.** Collapsing the three tiers; migrating runtime services to C++ as a product-path strategy; preserving obsolete product-facing APIs; a large reactive UI engine or heavy widget inheritance hierarchy; B2B vs B2C as framework runtime modes.

**Principles.** One strong default path; keep implementations small; add abstraction only when it removes real app burden; one fixed scaffold; runtime services stay in C, product composition in the framework.

**Validation.** Host smokes, scaffold checks, and CI expectations: **[docs/app/validation-host-loop.md](docs/app/validation-host-loop.md)**. Unified `blusys build` / `blusys qemu` (chip, `host`, `qemu*`): **[docs/app/cli-host-qemu.md](docs/app/cli-host-qemu.md)**.

---

## Quick start

```sh
# One-time install
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
~/.blusys/install.sh

# New project (default interface: handheld)
mkdir ~/my_product && cd ~/my_product
blusys create

# Explicit shape — see: blusys create --list
# blusys create --interface headless --with connectivity,telemetry my_sensor

# Build, flash, and monitor (from the project directory)
blusys run /dev/ttyACM0 esp32s3
```

ESP-IDF **v5.5+** is required; the CLI discovers it. For the full walkthrough — product shape, connected products, host loop — start at **[Getting started](docs/start/index.md)**.

---

## Architecture

One ESP-IDF component (`components/blusys/`). Pure concept code on the left, escape hatches on the right, C runtime at the bottom:

```mermaid
flowchart TB
    subgraph prod["Product · main/"]
        direction LR
        core["<b>core/</b><br/><sub>state · actions · reducer</sub>"]
        pui["<b>ui/</b><br/><sub>screens · widgets</sub>"]
        mplat["<b>platform/</b><br/><sub>wiring · capabilities</sub>"]
    end

    subgraph fw["Framework · C++ (pure)"]
        direction TB
        pure["<b>app · capabilities · flows</b><br/><b>engine · feedback · ui</b>"]
        fplat["<b>framework/platform/</b><br/><sub>escape hatch</sub>"]
        pure --> fplat
    end

    subgraph rt["Runtime · C"]
        direction LR
        svc["<b>Services</b><br/><sub>wifi · mqtt · storage · ota</sub>"]
        drv["<b>Drivers</b><br/><sub>button · lcd · dht · panels</sub>"]
        hal["<b>HAL</b><br/><sub>gpio · spi · i2c · adc · timer</sub>"]
        svc --> drv
        drv --> hal
        svc --> hal
    end

    idf([ESP-IDF · silicon])

    core  --> pure
    pui   --> pure
    mplat --> pure
    fplat --> rt
    mplat -. escape .-> rt
    hal   --> idf

    classDef pure   fill:#dbeafe,stroke:#1e3a8a,color:#1e3a8a,stroke-width:2px
    classDef escape fill:#fde68a,stroke:#92400e,color:#78350f,stroke-width:2px
    classDef cc     fill:#a7f3d0,stroke:#065f46,color:#064e3b,stroke-width:2px
    classDef idf    fill:#e5e7eb,stroke:#374151,color:#111827

    class core,pui,pure pure
    class mplat,fplat escape
    class svc,drv,hal cc
    class idf idf
```

**Rules** (enforced by `blusys lint`):
- `hal` depends on nothing above it. `drivers` → `hal`. `services` → `hal` + `drivers`.
- Pure framework (`app`, `capabilities`, `flows`, `engine`, `feedback`, `ui`) cannot include any lower layer. Only `framework/platform/` may bridge downward.
- Product `core/` and `ui/` are framework-only (pure). `main/platform/` is the product-level escape hatch and may include any layer directly.

| Layer | Role | Doc entry |
|-------|------|-----------|
| **Framework** | Product API, views, capabilities, host/device profiles | [App](docs/app/index.md) |
| **Services** | Runtime modules (connectivity, storage, system) | [Services](docs/services/index.md) |
| **Drivers** | Sensors, displays, and higher-level driver helpers | [HAL](docs/hal/index.md) |
| **HAL** | Registers, buses, and peripheral abstractions | [HAL](docs/hal/index.md) |

Module and example indices live in **`inventory.yml`** (CI and classification source of truth).

---

## Usage snippets

**Product code (recommended):**

```cpp
#include "blusys/framework/app/app.hpp"
```

**HAL and services (direct):**

```c
#include "blusys/blusys.h"   // umbrella: hal + drivers + services
```

**CMake `REQUIRES`:**

```cmake
idf_component_register(SRCS "main.c"   REQUIRES blusys)
idf_component_register(SRCS "main.cpp" REQUIRES blusys)
```

Widget authoring: **[widget-author-guide.md](components/blusys/widget-author-guide.md)**.

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
| `examples/quickstart/` | Canonical starters (inventory `ci_pr`), aligned with interface/capability examples |
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

Current **package version** is **7.0.0** (`BLUSYS_VERSION_STRING` in [`components/blusys/include/blusys/hal/version.h`](components/blusys/include/blusys/hal/version.h); also `blusys version`).

---

## License

See **[LICENSE](LICENSE)**.
