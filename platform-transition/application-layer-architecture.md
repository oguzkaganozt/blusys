# Application Layer Architecture

Blusys is no longer a reusable library collection. It is an **internal embedded product platform** for our product family.

One-line definition:

> Blusys is an internal embedded product platform that lets us build many products from one spine: HAL, drivers, and services stay in C; a C++ framework layer ships shared runtime abstractions and the widget kit; and product repos focus on capabilities, flows, assets, and business logic.

This document defines the application-layer architecture for that platform. For the full decision log see [`DECISIONS.md`](DECISIONS.md). For the C++ language policy see [`CPP-transition.md`](CPP-transition.md). For the UI system see [`UI-system.md`](UI-system.md).

## Platform goals

- Stop rethinking low-level wiring on every new product.
- Let product teams focus on business logic and prototyping speed.
- Share runtime, services, UI, and architectural rules across products.
- Build many products from one spine.

Platform promise:

- fast product building
- reliable spine
- consistent product quality

## Three-tier layer model

The platform has three tiers. Each tier has a fixed language and a clear purpose.

| Tier | Component | Language | Purpose |
|---|---|---|---|
| 1 | `components/blusys/` | C | HAL (direct MCU peripheral abstraction) + hardware drivers (composed over HAL) |
| 2 | `components/blusys_services/` | C | Stateful runtime: connectivity, protocols, system services, LVGL runtime |
| 3 | `components/blusys_framework/` | C++ | App framework: router, intent, feedback, controllers, widget kit |

Dependency direction is strictly downward:

```text
blusys_framework  →  blusys_services  →  blusys (HAL + drivers)
```

Reverse dependencies are forbidden. HAL and drivers know nothing about services. Services know nothing about framework. Framework knows nothing about product code.

### Tier 1 — HAL + drivers (`components/blusys/`)

The lowest tier contains both MCU peripheral abstractions (HAL) and hardware drivers composed over that HAL. Both are in C, both live in the same component, and both expose `extern "C"` headers.

**HAL** — direct abstraction over MCU peripherals. Wraps ESP-IDF's hardware drivers behind a smaller, more stable API.

- Sources: `src/common/*.c`
- Public headers: `include/blusys/<module>.h`
- Modules: `gpio`, `uart`, `spi`, `i2c`, `adc`, `dac`, `pwm`, `mcpwm`, `timer`, `pcnt`, `rmt`, `i2s`, `twai`, `wdt`, `sleep`, `temp_sensor`, `touch`, `sdmmc`, `sd_spi`, `nvs`, `one_wire`, `efuse`, `usb_host`, `usb_device`, and related.

HAL is explicitly stateless where possible (GPIO is pin-based, no handle) and uses opaque handles for stateful peripherals.

**Drivers** — hardware drivers composed over HAL. These modules drive external chips or compose HAL primitives into higher-level behavior.

- Sources: `src/drivers/<category>/*.c`
- Public headers: `include/blusys/drivers/<category>/<module>.h`
- Categories:
  - **display**: `lcd`, `led_strip`, `seven_seg`
  - **input**: `button`, `encoder`
  - **sensor**: `dht`
  - **actuator**: `buzzer`

Drivers call the public HAL API only. They never include `blusys/internal/**` except the explicit shared utilities (`blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h`). A CI lint check enforces this — see [`DECISIONS.md`](DECISIONS.md) decision 16.

Why drivers live inside the HAL component instead of their own: an earlier draft split drivers into `components/blusys_drivers/`. On review, this was ceremony without enforcement upside — directory discipline + CI lint catch the same boundary violations that a component split would, and collapsing to three tiers removes one CMakeLists, one umbrella header, one docs section, and ~10 example REQUIRES updates per release. See [`DECISIONS.md`](DECISIONS.md) decision 4.

### Tier 2 — Services (`components/blusys_services/`)

Stateful runtime modules: connectivity, protocols, system services, and runtime adapters such as the LVGL runtime and `usb_hid`.

- Language: C
- Public headers: C with `extern "C"`
- Categories:
  - **input/runtime**: `usb_hid`
  - **connectivity**: `wifi`, `wifi_prov`, `wifi_mesh`, `espnow`, `bluetooth`, `ble_gatt`, `mdns`
  - **protocol**: `mqtt`, `http_client`, `http_server`, `ws_client`
  - **system**: `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl`
  - **display** (runtime only): `ui` — LVGL init, render task, LCD flush integration, `blusys_ui_lock`

Services depend on HAL + drivers (`REQUIRES blusys`), never on framework. Services are where stateful orchestration, callback handling, and reconnect logic live. Services stays C in V1 — migration to C++ is deferred to V2 (see [`DECISIONS.md`](DECISIONS.md) decision 1).

Service-tier boundary discipline is intentionally pragmatic in V1. Drivers have a hard linted boundary inside `components/blusys/`; services do not. Existing services may keep using shared internal helpers where that keeps the platform simple for a small internal team.

Ownership notes:
- The `ui` module in services owns LVGL lifecycle, render task, and display flush. It does not own UI primitives — those live in the framework tier.
- `usb_hid` stays in services because it spans USB-host and BLE-HOGP transports, singleton runtime state, and BT/NVS bring-up helpers. It behaves like a runtime module more than a simple driver.
- `bluetooth`, `ble_gatt`, `espnow`, and `sntp` are singleton modules; only one instance at a time.

### Tier 3 — Framework (`components/blusys_framework/`)

The shared app framework: router, intent, feedback, controllers, widget kit. This is what product teams interact with most directly. It is named `blusys_framework` (not `blusys_app`) to avoid conflict with the product-side `app/` directory.

- Language: C++
- Public headers: C++
- Internal split:
- `blusys_framework/core/` — headless/app behavior: router, intent, feedback, controller base, fixed-capacity containers. Always available.
- `blusys_framework/ui/` — optional widget kit, theme tokens, draw helpers, input helpers. Only built when the product declares UI capability.

Framework depends on services (and transitively on drivers and HAL). It never depends on product code.

`blusys_framework/core` is intentionally mandatory even for headless products. That is a deliberate platform tradeoff: headless products do not get a different architectural rule set just because they lack a display. The platform standardizes lifecycle, controllers, intents, feedback, and app structure across the whole product line; only the UI subsystem is optional.

V1 scope:

**Core (`blusys_framework/core/`):**
1. Router contract with the 6-command set (`set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`) plus the `route_command` payload struct
2. Intent enum and dispatch (`press`, `confirm`, `increment`, `decrement`, `focus_next`, `focus_prev`, `long_press`, `cancel`)
3. Feedback channel with pluggable sinks (visual / audio / haptic)
4. Controller lifecycle base (`init` / `handle` / `tick` / `deinit`)
5. Fixed-capacity container set (see [`CPP-transition.md`](CPP-transition.md))

**UI (`blusys_framework/ui/`):**
6. `theme_tokens` struct and `set_theme()` / `theme()` accessors
7. Layout primitives (composition factories)
8. Interactive widgets (Camp 2 stock-backed or Camp 3 custom class — see [`UI-system.md`](UI-system.md))
9. Draw helpers
10. Input helpers

Deferred to V2: theme JSON pipeline, canvas-based specialty widgets, overlay manager as a distinct subsystem, reusable helpers beyond widgets.

Framework admission rule: a feature enters `blusys_framework` only if it is intentionally generic with low variance across products.

## Public surface

Include namespace `blusys/...` is preserved everywhere. Umbrella headers:

- `blusys/blusys.h` — HAL + drivers (C)
- `blusys/blusys_services.h` — services (C)
- `blusys/framework/framework.hpp` — framework (C++)

`blusys/blusys_all.h` is removed at the end of the transition. Code that currently includes it must move to the per-tier umbrella headers. The removal sweep covers 13 files (15 total occurrences):

- `components/blusys_services/include/blusys/blusys_all.h` — the header file itself
- **Examples (1 file):** `examples/ui_basic/main/ui_basic_main.c` (`examples/lcd_st7735_basic/main/lcd_st7735_basic_main.c` was already cleaned during Phase 3)
- **Guides (12 files, 14 occurrences):** `docs/guides/ota-basic.md`, `button-basic.md`, `http-server-basic.md`, `console-basic.md`, `ws-client-basic.md`, `mqtt-basic.md`, `lcd-basic.md` (×3), `fatfs-basic.md`, `fs-basic.md`, `http-basic.md`, `ui-basic.md`, `wifi-prov-basic.md`
- The scaffold template inside the `blusys` CLI (generated `main/app_main.cpp`) — already handled in Phase 7

The full call-site list with line numbers lives in [`ROADMAP.md`](ROADMAP.md) Phase 8.

## Language policy

Normative reference: [`CPP-transition.md`](CPP-transition.md).

Summary:
- HAL, drivers, and services: C, `extern "C"` public headers.
- Framework: C++ with Classes, `-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics`. The only C++ tier in V1.
- Allocation: fixed-capacity containers + caller-owned buffers. No dynamic allocation in steady state.
- Thread safety: FreeRTOS primitives only, never hold locks across blocking waits.
- Logging in framework code via the thin `blusys/log.h` wrapper (see [`DECISIONS.md`](DECISIONS.md) decision 14).

## Product configuration

The platform does not ship a fixed "everything comes bundled" stack — headless products skip the widget kit, interactive products build it. V1 keeps the choice intentionally simple: **one starter type selects the full component set, and linker dead-stripping handles the rest.**

### `product_config.cmake`

Each product repo declares its product-facing configuration in `app/product_config.cmake`, scaffolded once and committed to the repo. This is the file the product team edits to declare what the product is.

```cmake
set(BLUSYS_PRODUCT_NAME  "my_product")
set(BLUSYS_STARTER_TYPE  "interactive")   # "headless" or "interactive"
set(BLUSYS_TARGETS       "esp32s3" "esp32c3")
```

Those three fields are the primary scaffold inputs in V1. They define what the product is at creation time, but they are not a promise that every generated file stays auto-derived forever.

The scaffold generates additional build-system files from `BLUSYS_STARTER_TYPE` at create time:

| Generated file | What the starter type controls |
|---|---|
| Top-level `CMakeLists.txt` | Sets `BLUSYS_BUILD_UI` CACHE variable (`ON` for interactive, `OFF` for headless) — see decision 15 |
| `app/CMakeLists.txt` | Adds `ui/`, `ui/screens/`, `ui/widgets/` to `SRC_DIRS` when `BLUSYS_BUILD_UI=ON` |
| `main/idf_component.yml` | Always pins `blusys`, `blusys_services`, `blusys_framework`; additionally pins `lvgl/lvgl` when the starter is `interactive` |
| `app/ui/theme_init.cpp`, sample screens | Generated only for interactive |

These generated files are committed to the product repo after scaffold and then live as part of the product's source tree — they are not regenerated on `idf.py reconfigure`. If a product later switches starter type, the product team should expect to re-scaffold or hand-edit the affected files. V1 intentionally treats scaffold output as normal source, not as a live sync/regenerate system.

The components linked are the same in both profiles:

| Starter       | Components linked                               | `blusys_framework/ui` built? | Managed deps differ |
|---------------|-------------------------------------------------|------------------------------|---------------------|
| `headless`    | `blusys`, `blusys_services`, `blusys_framework` | No                           | no lvgl             |
| `interactive` | `blusys`, `blusys_services`, `blusys_framework` | Yes                          | + `lvgl/lvgl`       |

Both profiles link all three tiers. This is intentional: headless products are still platform products and therefore still use `blusys_framework/core`. Unused modules within a linked component cost almost nothing in flash thanks to linker dead-stripping (`-ffunction-sections -fdata-sections -Wl,--gc-sections`, ESP-IDF's default). A headless product that never touches `blusys/drivers/display` pays nothing for LCD code; an interactive product that uses only `wifi` and `mqtt` pays nothing for `bluetooth` or `ble_gatt`.

The `blusys_framework/ui` gate is implemented via a `BLUSYS_BUILD_UI` CACHE variable set by the scaffold-generated product top-level CMakeLists.txt before `project()`; the framework's CMakeLists reads that variable to decide whether to compile the widget kit sources. See [`DECISIONS.md`](DECISIONS.md) decision 15 for the full mechanism.

### Why just starter type in V1

Earlier drafts defined a `BLUSYS_CAPABILITIES` list enumerating `display`, `input`, `connectivity`, `protocol`, `ui`, and so on. That list has been removed from V1.

The build system in V1 is **component-granular**: either a component is in the `REQUIRES` list or it is not. The capability list would have done one thing — decide which of the three components to pull in — and that mapping is trivial from the starter type alone. For a small internal team, a capability vocabulary without an underlying build mechanism is ceremony, not modularity.

Per-module build gating (e.g., "link `blusys_services` but omit the `bluetooth` module entirely") is a **V2 concern**. It will be implemented via Kconfig symbols in each component's `Kconfig.projbuild`, following the same pattern ESP-IDF uses for its optional components. When that lands, a structured declaration will be added back to `product_config.cmake` — but driven by a real build mechanism, not as documentation.

Until then, V1 is honest about what the build system actually enforces: starter type decides whether `blusys_framework/ui` is built, and dead-stripping handles the rest.

### Starter profiles

Two official starter profiles:

**`headless`** — no display, no interaction surface.

```text
app/
  CMakeLists.txt      (ESP-IDF component, SRC_DIRS controllers + integrations)
  product_config.cmake
  controllers/
  integrations/
  config/
```

No `app/ui/`, no `theme_init`, no theme tokens. `blusys_framework/core` is still built on purpose; `blusys_framework/ui` is not.

**`interactive`** — display and UI capability.

```text
app/
  CMakeLists.txt      (ESP-IDF component, SRC_DIRS adds ui/ + ui/screens/ + ui/widgets/ when BLUSYS_BUILD_UI=ON)
  product_config.cmake
  ui/
    theme_init.cpp
    screens/
    widgets/        (product-specific widgets — compiled if present, directory is scaffolded empty)
  controllers/
  integrations/
  config/
```

Both `blusys_framework/core` and `blusys_framework/ui` are built.

**`app/` is an ESP-IDF component.** Product sources under `app/ui/`, `app/controllers/`, `app/integrations/`, and `app/ui/widgets/` belong to the `app` component; the scaffolded `app/CMakeLists.txt` registers them via `idf_component_register(SRC_DIRS ...)`. `main/` remains the auto-discovered entry component with `main/CMakeLists.txt` declaring `REQUIRES app`. The top-level project `CMakeLists.txt` sets `EXTRA_COMPONENT_DIRS = "${CMAKE_CURRENT_LIST_DIR}/app"` so ESP-IDF discovers `app/` as a single component during its component scan. (Phase 7 implementation note: pointing the variable at the project root would make ESP-IDF's `__project_component_dir` helper at `tools/cmake/project.cmake:432` treat the project root as a component itself, recursively re-including the top-level CMakeLists.txt during requirement scanning. The path is therefore scoped to `app/`.)

Because `app/CMakeLists.txt` declares `INCLUDE_DIRS "."`, files inside the app component (and files in `main/` that declare `REQUIRES app`) use include paths relative to the `app/` directory — e.g. `#include "controllers/home_controller.hpp"`, not `#include "app/controllers/home_controller.hpp"`.

Bootstrap entry point is `main/app_main.cpp` in both profiles (always `.cpp`, because both profiles link the C++ `blusys_framework`). This is part of the same standardization choice: headless products still enter the platform through the framework core.

See [`DECISIONS.md`](DECISIONS.md) decision 15 for the full CMake template set and [`ROADMAP.md`](ROADMAP.md) Phase 7 for the scaffold work.

## Multi-repo consumption

Product repos consume the platform as **ESP-IDF managed components with git sources**. Each product's `main/idf_component.yml` pins all three components to the **same platform version tag**:

```yaml
dependencies:
  blusys:
    git: "git@github.com:oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys"
  blusys_services:
    git: "git@github.com:oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys_services"
  blusys_framework:
    git: "git@github.com:oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys_framework"
```

### Single platform version rule

**All three components in a product's `idf_component.yml` must pin the same version tag.** A product that pins `blusys@v5.0.0` must also pin `blusys_services@v5.0.0` and `blusys_framework@v5.0.0`. Mixing tiers across versions is **not supported**.

Rationale: all three components live in the same git repo. A release tag (e.g., `v5.0.0`) marks one specific commit. If a product mixes tiers from different tags, it is silently mixing components built against different internal contracts — the HAL symbols, service signatures, and framework headers were not tested together. The result is undefined-behavior territory at link or run time.

The scaffold (`blusys create`) generates all three dependencies pinned to the same tag. Product teams upgrade by bumping **all three** version fields to the new tag simultaneously and running `idf.py reconfigure`. There is no supported workflow for bumping one tier without the others.

Per-tier version independence is **explicitly a non-goal**. This is a monorepo platform shipped as one versioned unit, not three independently-evolving libraries.

### Release versioning

Platform releases use semver at the repo level:
- **Major** (`v5` → `v6`): breaking changes to any public API (HAL, drivers, services, framework)
- **Minor** (`v5.0` → `v5.1`): additive changes, new capabilities, new widgets
- **Patch** (`v5.0.0` → `v5.0.1`): bug fixes, no API changes

A single tag covers all three components. This uses ESP-IDF's native dependency resolver — no registry infrastructure required.

## Dependency direction at the application level

```text
product UI           →  product controllers
product controllers  →  platform services  →  platform HAL+drivers
product controllers  →  product integrations  →  platform services  →  ...
```

Rules:

- Screens never call services directly. Screens render from props, raise events to controllers.
- Controllers own state transitions and make route decisions.
- Controllers may call simple single-capability platform APIs directly (read a sensor value, toggle an LED).
- Multi-capability composition, callback-heavy flows, and product-specific platform adapters go into `app/integrations/`.
- Services never depend on product code, ever.
- HAL + drivers never know about upper layers.

## Controller model

Normative API:

```cpp
struct controller {
    blusys_err_t init();
    void         handle(const app_event& e);
    void         tick(uint32_t now_ms);
    void         deinit();
};
```

- Controllers are small and domain-scoped. Avoid mega-controllers.
- `tick()` is called by the framework at a configurable cadence (default 10 ms). Use it for polling, timeouts, time-based feedback, and state-tied animations.
- Controllers produce router commands; they do not manipulate widgets directly.

## Router contract

Minimum command set:

- `set_root(route_id)` — replace the navigation stack with a new root screen
- `push(route_id, params?)` — push a new screen on top of the current one
- `replace(route_id, params?)` — replace the current screen
- `pop()` — return to the previous screen
- `show_overlay(overlay_id, params?)` — show a transient overlay (modal, toast, feedback surface)
- `hide_overlay(overlay_id)` — hide a specific overlay

`route_command` payload:

```cpp
struct route_command {
    enum type_t { set_root, push, replace, pop, show_overlay, hide_overlay } type;
    uint32_t    id;           // route_id or overlay_id
    const void* params;       // optional, pointer to product-defined struct
    uint32_t    transition;   // optional transition hint
};
```

Products do not invent their own router vocabulary. They consume the platform command set.

Feedback is a separate channel from the router. Headless products do not emit router commands by default.

## Intent model

Input is normalized to intent-level events at the framework capability adapter layer. The framework defines the core intent enum:

```cpp
enum class intent : uint8_t {
    press, long_press, release,
    confirm, cancel,
    increment, decrement,
    focus_next, focus_prev,
};
```

Hardware-specific events (encoder tick, touch press, button release) are mapped to intents by the framework's input adapters. Controllers consume intents, not raw hardware events.

## UI system

See [`UI-system.md`](UI-system.md) for full detail. Summary:

- LVGL is the rendering, layout, and event engine. The platform leverages LVGL; it does not replace it.
- The widget kit is custom-built on top of LVGL primitives. Normal product screen code uses platform widgets first; raw LVGL stays available for lower-level helpers and product-specific widgets when needed.
- The kit has two categories: **layout primitives** (composition factories over `lv_obj_create`) and **interactive widgets** (stock-backed or custom `lv_obj_class_t` subclasses).
- Every widget reads visual values from a single `blusys::ui::theme_tokens` struct populated by the product at boot.
- Widget implementation can be Camp 2 (stock-LVGL-backed) or Camp 3 (custom class). The choice is per-widget and invisible to product code.
- V1 reduces LVGL exposure in normal product code, but does not try to hide LVGL completely. Opaque handles and engine-native value types may still appear in framework headers where that keeps the API light.

## Theme model

No JSON, no Python, no build-time generation.

```cpp
namespace blusys::ui {
    struct theme_tokens {
        lv_color_t color_primary, color_surface, color_on_primary,
                   color_accent, color_disabled;
        int spacing_sm, spacing_md, spacing_lg;
        int radius_card, radius_button;
        const lv_font_t *font_body, *font_title, *font_mono;
    };
    void set_theme(const theme_tokens& t);
    const theme_tokens& theme();
}
```

Products populate this struct once at boot. Every widget reads `theme()` when it draws. The struct is append-only: new fields have default values, old products keep building.

Using `lv_color_t` and `lv_font_t*` in `theme_tokens` is intentional. They are LVGL-native value and handle types, not a sign that the widget API has failed to abstract normal product behavior.

## Thread safety

- FreeRTOS primitives only (via `blusys/internal/blusys_lock.h` in HAL, via platform-provided lock types in services/framework).
- Never hold `blusys_lock_t` or `blusys_ui_lock` across a blocking wait.
- Service callbacks never mutate UI widgets or application state directly. They post events to the controller layer.
- Controllers own all state transitions.

## Non-goals

- Porting HAL to C++.
- Migrating services to C++ in V1 (deferred to V2 per [`DECISIONS.md`](DECISIONS.md) decision 1).
- Building a heavy C++ framework with RAII over HAL and LVGL handles.
- Absorbing product business logic into the platform.
- Forcing every product into the same UI style or capability set.
- Requiring a JSON theme pipeline, Python tooling, or a design-tool integration in V1.

## Summary

- Blusys is an internal embedded product platform.
- Three tiers: HAL + drivers (C, in `components/blusys/`) → services (C, in `components/blusys_services/`) → framework (C++, in `components/blusys_framework/`).
- HAL/drivers boundary inside `components/blusys/` is enforced by directory discipline + CI lint, not by a component boundary.
- Framework is the only C++ tier in V1. Services migration to C++ is a V2 concern.
- Product teams consume the platform via ESP-IDF managed components with semver pinning.
- Product configuration: `product_config.cmake` declares starter type (`headless` or `interactive`); per-module gating is deferred to V2.
- Scaffold behavior is intentionally one-time in V1: `product_config.cmake` is the primary starter input, but generated files are normal source after creation.
- `headless` and `interactive` starter profiles. UI gate via `BLUSYS_BUILD_UI` CACHE variable.
- Framework uses C with Classes, fixed-capacity allocation, FreeRTOS primitives.
- LVGL is the UI engine; the widget kit is custom-built on top.
- Theme is a single struct populated at boot — no JSON, no tooling.
- The framework is the shared spine; products own capabilities, flows, assets, and business logic.

This is Blusys's evolution from a library repo into a product platform.
