# Platform Transition Roadmap

This document breaks down Blusys's transition from a reusable HAL/services repo into an internal embedded product platform.

For the locked architectural decisions that shape this roadmap see [`DECISIONS.md`](DECISIONS.md). For the broader architecture see [`application-layer-architecture.md`](application-layer-architecture.md). For the C++ policy see [`CPP-transition.md`](CPP-transition.md). For the UI system see [`UI-system.md`](UI-system.md).

## Core decisions (already locked)

- HAL, drivers, and services: C. Framework: C++. Services migration to C++ is deferred to V2.
- Three-tier layer model: `blusys` (HAL + drivers, C) → `blusys_services` (C) → `blusys_framework` (C++).
- Drivers live inside `components/blusys/` under `src/drivers/<category>/` and `include/blusys/drivers/<category>/`. HAL/drivers boundary is enforced by directory discipline + CI lint, not by a component boundary.
- Blusys is positioned as an internal embedded product platform.
- Product configuration in V1 is driven by a single starter type (`headless` or `interactive`); per-module build gating is deferred to V2.
- `BLUSYS_BUILD_UI` CACHE variable gates `blusys_framework/ui` compilation.
- UI is an optional subsystem.
- Widget kit is custom-built on LVGL. No stock widgets in product code.
- Theme is a single struct populated at boot. No JSON, no Python.
- Product repos consume the platform as ESP-IDF managed components with git sources from `oguzkaganozt/blusys`.
- Framework logging goes through the thin `blusys/log.h` wrapper.

For the complete set of 16 decisions see [`DECISIONS.md`](DECISIONS.md).

## Target end state

```text
Blusys platform repo
  components/blusys/                HAL + drivers (C)
    src/common/                     HAL sources
    src/drivers/<category>/         driver sources
    include/blusys/<module>.h       HAL public headers
    include/blusys/drivers/<...>    driver public headers
    include/blusys/log.h            thin esp_log wrapper
    include/blusys/internal/        shared utilities (locks, error, timeout)
  components/blusys_services/       stateful runtime (C, unchanged in V1)
  components/blusys_framework/      app framework + widget kit (C++)
    src/core/                       router, intent, feedback, containers
    src/ui/                         theme, primitives, widgets, draw, input
    include/blusys/framework/       public C++ headers

Product repos
  CMakeLists.txt                    top-level — reads product_config.cmake,
                                    sets BLUSYS_BUILD_UI, EXTRA_COMPONENT_DIRS="."
  sdkconfig.defaults
  main/
    CMakeLists.txt                  idf_component_register SRCS app_main.cpp, REQUIRES app
    app_main.cpp                    extern "C" bootstrap (always .cpp)
    idf_component.yml               managed deps: blusys, blusys_services, blusys_framework
  app/                              local ESP-IDF component (discovered via EXTRA_COMPONENT_DIRS)
    CMakeLists.txt                  idf_component_register SRC_DIRS (UI dirs gated on BLUSYS_BUILD_UI)
    product_config.cmake            BLUSYS_PRODUCT_NAME, BLUSYS_STARTER_TYPE, BLUSYS_TARGETS
    ui/                             screens, theme_init (interactive only)
    controllers/                    state, intent handling, route decisions
    integrations/                   multi-capability composition, product adapters
    config/                         product settings
```

Platform value:
- Low-level wiring is solved once, reused across products.
- Product teams write business logic, not peripheral glue.
- Shared runtime, UI, and feedback language across products.
- Many products from one spine.

## Phases

### Phase 1 — Lock platform positioning

**Goal:** Close the "library vs platform" debate. Lock architectural decisions normatively.

**Deliverables:**
- [`DECISIONS.md`](DECISIONS.md) — canonical log of all locked decisions
- [`application-layer-architecture.md`](application-layer-architecture.md) — primary architecture doc
- [`CPP-transition.md`](CPP-transition.md) — C++ language policy
- [`UI-system.md`](UI-system.md) — UI architecture and widget contract
- This roadmap

**Done when:**
- Architectural decisions are normative, not discussion notes.
- New contributors can read one doc and understand the platform's shape.

### Phase 2 — Identity alignment

**Goal:** Update the repo's top-level identity from "library" to "platform."

**Scope (narrow by design):** only files that carry the top-level identity framing. No layering prose, component diagrams, or migration notes — those wait until Phase 3.

Touch list:

1. **`README.md` lines 1-5** — change title and tagline from "Blusys HAL / A simplified C hardware abstraction layer..." to "internal embedded product platform" framing.
2. **`README.md` line 85** — the "Current Release" callout currently says "54 modules across HAL and Services." Drop the module count entirely (it is about to become obsolete in Phase 3) and reframe the callout as a transition status line pointing at `PROGRESS.md`.
3. **`docs/index.md` lines 1-3** — matching heading and one-paragraph description change.
4. **`docs/index.md` line 44** — "Current Release" admonition currently says "62 modules across HAL and Services." Same treatment: drop the count, reframe as transition status.
5. **`docs/architecture.md` lines 1-3** — change only the intro paragraph to reference the three-tier model at a high level. Do not touch the rest of the file; the body rewrite is Phase 3.
6. **`mkdocs.yml` lines 1-2** — `site_name: Blusys HAL` → `site_name: Blusys Platform` (or equivalent); update `site_description` to match.
7. **`CLAUDE.md` line 7** — project-instructions header currently says "**Blusys HAL** is a simplified C Hardware Abstraction Layer... 62 modules across HAL and Services." Update the framing and drop the stale count.

**Deliverables:**
- Top-level identity (README, docs landing page, mkdocs site metadata, CLAUDE.md) reads "platform" instead of "library/HAL" everywhere.
- Stale module counts (54 in README, 62 elsewhere) are removed rather than updated — the correct count will land naturally in Phase 3 when the packaging prose is rewritten.
- `docs/architecture.md` body, `docs/guidelines.md`, `docs/target-matrix.md`, module/guide card grids, and mkdocs nav sub-sections are **not touched** in this phase.

**Done when:**
- A reader of the top-level README or the docs landing page comes away with "platform" as the repo's positioning.
- No single file in the narrow touch list still uses "library," "HAL" as the primary framing, or the stale module count.

**Explicitly not in scope:** detailed layering docs, component diagrams, module migration prose, card grid restructures, `docs/guidelines.md` language policy rewrite, mkdocs nav restructure. These are Phase 3.

### Phase 3 — Target packaging shape

**Goal:** Decide and document the physical repo shape of the platform.

**Work:**
- **Move drivers into `components/blusys/`** (no new component):
  - Move sources from `components/blusys_services/src/<display|input|sensor|actuator>/` to `components/blusys/src/drivers/<category>/`.
  - Move public headers from `components/blusys_services/include/blusys/<category>/` to `components/blusys/include/blusys/drivers/<category>/`.
  - Affected modules: `lcd`, `led_strip`, `seven_seg`, `button`, `encoder`, `dht`, `buzzer`.
  - The `ui` and `usb_hid` runtime modules stay in `blusys_services/` — they are stateful runtime modules, not simple hardware drivers.
- **Update include paths in the 9 driver examples** from `blusys/<category>/<module>.h` to `blusys/drivers/<category>/<module>.h`. Affected examples: `lcd_basic`, `lcd_ssd1306_basic`, `lcd_st7735_basic`, `led_strip_basic`, `seven_seg_basic`, `button_basic`, `encoder_basic`, `dht_basic`, `buzzer_basic`.
- **Update include paths in 5 docs/guides/ files** that reference driver headers directly (same mechanical edit as the examples):
  - `docs/guides/led-strip-basic.md:16` — `blusys/display/led_strip.h`
  - `docs/guides/seven-seg-basic.md` lines 22, 45, 70 — `blusys/display/seven_seg.h` (three occurrences)
  - `docs/guides/encoder-basic.md:17` — `blusys/input/encoder.h`
  - `docs/guides/dht-basic.md:21` — `blusys/sensor/dht.h`
  - `docs/guides/buzzer-basic.md:19` — `blusys/actuator/buzzer.h`
- **Update driver examples' REQUIRES** from `blusys_services` to `blusys` (same 9 examples).
- **Update `blusys_services/src/display/ui.c`** to include `blusys/drivers/display/lcd.h` instead of `blusys/display/lcd.h`.
- **Update `components/blusys/CMakeLists.txt`** to append driver sources to `blusys_sources` and add `esp_lcd` to its `REQUIRES` list (for the `lcd` driver).
- **Update `components/blusys_services/CMakeLists.txt`** to remove driver sources and the now-unneeded `esp_lcd` REQUIRES entry.
- **Update `components/blusys/include/blusys/blusys.h`** umbrella header to also include driver headers.
- **Add the CI lint check** (`scripts/lint-layering.sh` or similar, ~30 lines) enforcing:
  - No file under `components/blusys/src/common/` includes `blusys/drivers/**`.
  - No file under `components/blusys/src/drivers/` includes `blusys/internal/**` except the allowlist (`blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h`).
  - No service-tier lint rule in V1 beyond normal code review; `blusys_services/` keeps pragmatic access to shared internals where needed.
  - Wire the check into the `blusys build-examples` path and/or as a standalone `blusys lint` subcommand.
- **Create `components/blusys_framework/`** as an empty C component stub (no C++ flags yet — Phase 4 adds them) with the `src/core/` and `src/ui/` subdirectories created and a placeholder source so `idf_component_register` succeeds.

**Docs rewrite (full layering prose lands here):**

- **Rewrite `docs/architecture.md`** with the full three-tier layering model, component diagram, dependency rules, and HAL/drivers CI lint explanation. This replaces the entire body of the file (Phase 2 only touched the intro).
- **Rewrite `docs/architecture.md` CMakeLists.txt Usage section** (currently lines 87-98): update `REQUIRES` examples so driver examples show `REQUIRES blusys` (not `blusys_services`) and remove `blusys_all.h` from the umbrella-header paragraph.
- **Split the Services section in `docs/target-matrix.md`** (currently lines 25-37) into three subsections: Drivers (display/input/sensor/actuator), Services (`usb_hid` runtime + connectivity/protocol/system), and Framework (appears once Phase 5 lands, but the section can be stubbed now).
- **Split the Services card grids** in `docs/modules/index.md` and `docs/guides/index.md`. Each currently has one "Services" section with 7 category cards; that becomes two sections ("Drivers" with display/input/sensor/actuator, "Services" with input/runtime + connectivity/protocol/system). A third "Framework" section is stubbed empty and filled in during Phase 5.
- **Restructure `mkdocs.yml` nav sub-sections**: both Guides (lines 102-136) and API Reference (lines 179-213) currently have a single `- Services:` sub-tree with 7 categories. Split each into `- Drivers:` (display/input/sensor/actuator) and `- Services:` (input/runtime + connectivity/protocol/system). Add placeholder `- Framework:` sub-trees for the widget kit pages landing later.
- **Update `docs/guidelines.md` API design rules** (line 8 currently says "public API is C only and must not expose ESP-IDF types"). Add a subsection explaining the language policy: HAL, drivers, and services expose C headers with `extern "C"`; framework exposes C++ headers. Cross-reference [`platform-transition/CPP-transition.md`](CPP-transition.md) for the full policy.
- **Run `mkdocs build --strict`** — the nav restructure is fragile, and this is the existing doc gate from CLAUDE.md.

**Deliverables:**
- Drivers live inside `components/blusys/` under the new `src/drivers/` structure.
- CI lint passes on the moved drivers and fails if layering is violated.
- `blusys_framework` component exists and compiles as an empty stub.
- Docs reflect the three-tier packaging — architecture, target matrix, module/guide card grids, mkdocs nav, and guidelines all consistent.
- All 9 driver examples and 5 affected guide files build against the updated include path.
- `mkdocs build --strict` passes.

**Done when:**
- Three components exist and build cleanly for all three targets.
- The question "where does this code go?" has a single answer for any given module.
- The layering lint check catches a manufactured violation (test it with a temporary bad include).

### Phase 4 — C++ infrastructure (framework only) + platform logging

**Goal:** Prepare the framework component for C++ development. Services and HAL/drivers are untouched.

**Work:**
- **Add C++ build policy** to `components/blusys_framework/CMakeLists.txt`:

```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -Os -ffunction-sections -fdata-sections -Wall -Wextra)
```
- **No C++ flags are added to `blusys` or `blusys_services`.** Those components stay C in V1.
- **Add `components/blusys/include/blusys/log.h`** — a thin wrapper over `esp_log.h` defining `BLUSYS_LOGE`, `BLUSYS_LOGW`, `BLUSYS_LOGI`, `BLUSYS_LOGD`. Framework code uses these macros; HAL and services continue using `esp_log.h` directly (no churn). See [`DECISIONS.md`](DECISIONS.md) decision 14 for the rationale and header contents.
- **Implement fixed-capacity container set** in `components/blusys_framework/include/blusys/framework/core/containers.hpp`:
  - `blusys::array<T, N>`
  - `blusys::string<N>`
  - `blusys::ring_buffer<T, N>`
  - `blusys::static_vector<T, N>`
- **Verify the allocation policy** (no `new`/`delete`, no dynamic allocation after init) is enforceable in code review.
- **Write a short `components/blusys_framework/README.md`** describing the `core/ui` split, naming conventions, and the `BLUSYS_BUILD_UI` gate mechanism.
- **Verify LTO can be enabled** on the framework component — with framework as the only C++ component, there is no hybrid C/C++ link-time uncertainty.

**Deliverables:**
- C++ compiles in the framework component.
- Container set is usable.
- A trivial framework `.cpp` file can use namespaces, `enum class`, and the container set successfully.
- `blusys/log.h` exists and is used by at least one framework source.

**Done when:**
- A new framework module can be written in C++ using only the allowed feature set and logs via `BLUSYS_LOGE` successfully.

### Phase 4.5 — PC + SDL2 host harness

**Goal:** Stand up a host-side LVGL build so the widget kit can be iterated on without flashing hardware for every change.

**Work:**
- Create `scripts/host/` (or `tools/host/`) with a minimal CMake project that builds LVGL against SDL2 on Linux.
- Pin the same LVGL version used by ESP-IDF managed components.
- Stub `blusys::ui::theme()` with a small default theme for host-side rendering.
- Add a "hello LVGL" host target that opens an SDL2 window and draws one `lv_obj_create` rect — confirms the toolchain works end-to-end before any widget code is written.
- Document the host build in `scripts/host/README.md` (how to install SDL2 dev headers, how to build, how to run).
- Add a `blusys host-build` CLI subcommand that shells into `scripts/host/` and runs the build.

**Deliverables:**
- Host build runs on Linux and shows an SDL2 window with a single LVGL rect.
- `blusys host-build` wraps the toolchain invocation.
- README documents the setup steps.

**Done when:**
- The host harness is reproducible from a fresh checkout with one command.
- Phase 5 widget work can validate on host before touching hardware.

**Rationale:** An earlier draft folded this work into Phase 5a as a side effect of the flagship widget's "works on PC+SDL2" criterion. Isolating it lets Phase 5a focus purely on widget design without surprise yak-shaving on SDL2 integration.

### Phase 5 — Flagship widget and V1 widget kit

**Goal:** Prove the widget contract works on one flagship widget before scaling, then fill out the V1 kit.

**Status:** In progress. `theme.hpp`, `widgets.hpp`, and the first layout primitives (`screen`, `row`, `col`, `label`, `divider`) are already landed, along with `examples/framework_ui_basic/`. Interactive widgets and the remaining primitives are still pending.

This phase has three sub-steps. The sub-steps exist specifically to de-risk the widget kit: build one widget carefully, validate the pattern, then scale. (A pilot service-migration sub-phase was removed in the pre-implementation refinement pass — services stays C in V1.)

#### Phase 5a — Flagship widget: `bu_button`

**Work:**
- Implement `bu_button` in `blusys_framework/src/ui/widgets/button/` as a Camp 2 stock-backed widget wrapping `lv_button`. Use the fail-loud slot pool pattern from [`UI-system.md`](UI-system.md).
- Populate `blusys_framework/include/blusys/framework/ui/theme.hpp` with the `theme_tokens` struct and `set_theme`/`theme()` accessors.
- Implement `blusys_framework/src/ui/draw/` helpers (`filled_rounded_rect`, `centered_text`, `focus_ring`).
- Use `BLUSYS_LOGE` from `blusys/log.h` for error logging (validates Phase 4 infrastructure).
- Over-invest in comments and review. This widget is the template every subsequent widget copies.
- Write a short `widget-author-guide.md` inside `blusys_framework/` documenting the pattern `bu_button` establishes.

**Done when:**
- `bu_button` works on PC+SDL2 (via Phase 4.5 harness) and ESP-IDF QEMU.
- The pattern it establishes is documented.
- A manufactured pool-exhaustion test fires the assert cleanly.

#### Phase 5b — Validate the pattern with 3 more widgets

**Work:**
- Implement `bu_label`, `bu_row`, `bu_screen` following the `bu_button` pattern (these are composition primitives, simpler than `bu_button`).
- If the pattern feels forced or repetitive while writing these, adjust `bu_button` and the guide before continuing.

**Done when:**
- All four widgets (`bu_button`, `bu_label`, `bu_row`, `bu_screen`) work on PC+SDL2.
- The author guide is stable.

#### Phase 5c — Build the rest of the V1 widget kit

**Work:**
- Implement the remaining layout primitives: `bu_col`, `bu_stack`, `bu_spacer`, `bu_divider`, `bu_text_value`.
- Implement the remaining Camp 2 interactive widgets: `bu_slider`, `bu_toggle`, `bu_modal`, `bu_overlay`.
- Implement `bu_knob` as Camp 3 **only if** a pilot product has declared it needs one. Otherwise defer to V2.
- Implement `blusys_framework/src/ui/input/` helpers (`create_encoder_group`, `auto_focus_screen`).

**Done when:**
- The full V1 kit compiles and works on PC+SDL2.
- The kit can be used to build a complete screen without touching LVGL directly.

### Phase 6 — Framework core V1

**Goal:** Ship the minimal spine alongside the widget kit so controllers can actually consume platform services.

**Status:** In progress. `router.hpp`, `intent.hpp`, `feedback.hpp`, `controller.hpp`, `runtime.hpp`, and `examples/framework_core_basic/` are already landed. Remaining work is to connect the spine to richer product flows and the incoming widget layer.

**Work (in parallel with Phase 5):**
- Define the router contract in `blusys_framework/core/router.hpp`:
  - Minimum command set: `set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`
  - `route_command` payload struct
- Define the intent enum and dispatch in `blusys_framework/core/intent.hpp`.
- Define the feedback channel with pluggable sinks in `blusys_framework/core/feedback.hpp`.
- Define the controller lifecycle base in `blusys_framework/core/controller.hpp`:
  - `init` / `handle` / `tick` / `deinit`
- Tick cadence: default 10 ms, configurable per product.
- Framework event loop/runtime in `blusys_framework/core/framework.cpp` / `runtime.hpp` (`blusys::framework::init`, `blusys::framework::run`, or equivalent runtime ownership API).

**Deliverables:**
- `blusys_framework/core` is usable by a controller that routes between screens and emits feedback.
- `blusys_framework/ui` is usable by screens that compose widgets.

**Done when:**
- A minimal interactive sample app runs end-to-end: encoder input → intent → controller → route command → screen update → feedback.

### Phase 7 — Product scaffold and sample app

**Goal:** Make starting a new product frictionless.

**Work:**
- Extend `blusys create [path]` in the `blusys` CLI to accept `--starter <headless|interactive>` (default: `interactive`).

**Scaffolded file set:**

```text
<product-root>/
  CMakeLists.txt              (top-level — reads product_config.cmake, sets BLUSYS_BUILD_UI, declares EXTRA_COMPONENT_DIRS=".")
  sdkconfig.defaults          (empty or minimal)
  main/
    CMakeLists.txt            (idf_component_register: SRCS app_main.cpp, REQUIRES app)
    app_main.cpp              (extern "C" void app_main(void) — always .cpp)
    idf_component.yml         (managed deps: blusys, blusys_services, blusys_framework, lvgl/lvgl if interactive)
  app/
    CMakeLists.txt            (idf_component_register: SRC_DIRS derived from BLUSYS_BUILD_UI, REQUIRES platform components)
    product_config.cmake      (BLUSYS_PRODUCT_NAME, BLUSYS_STARTER_TYPE, BLUSYS_TARGETS)
    controllers/
      home_controller.hpp     (sample)
      home_controller.cpp
    integrations/
      .keep
    config/
      .keep
    ui/                       (interactive only)
      theme_init.cpp          (populates blusys::ui::theme_tokens)
      screens/
        main_screen.cpp       (sample)
```

**Why `app/` is its own ESP-IDF component:** product sources under `app/ui/`, `app/controllers/`, `app/integrations/` must belong to *some* ESP-IDF component — `main/` is too narrow (one `idf_component_register` with a flat source list), and globbing across directories from `main/CMakeLists.txt` is fragile. The cleanest pattern is for `app/` to be a component in its own right, with `app/CMakeLists.txt` calling `idf_component_register(SRC_DIRS ...)`. The top-level `CMakeLists.txt` sets `EXTRA_COMPONENT_DIRS` to the project root so ESP-IDF discovers `app/` during its component scan. `main/` keeps its usual auto-discovered role and its `main/CMakeLists.txt` simply `REQUIRES app` to pull everything in.

**Top-level CMakeLists.txt template** (scaffold-generated verbatim):

```cmake
cmake_minimum_required(VERSION 3.16)

include(${CMAKE_CURRENT_LIST_DIR}/app/product_config.cmake)

if(BLUSYS_STARTER_TYPE STREQUAL "interactive")
    set(BLUSYS_BUILD_UI ON CACHE BOOL "Build blusys_framework/ui")
else()
    set(BLUSYS_BUILD_UI OFF CACHE BOOL "Build blusys_framework/ui")
endif()

# Discover the local app/ component. Platform components come from
# main/idf_component.yml via the managed component manager — NOT from
# $ENV{BLUSYS_PATH}. See DECISIONS.md decision 15.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${BLUSYS_PRODUCT_NAME})
```

**`app/CMakeLists.txt` template:**

```cmake
set(src_dirs "controllers" "integrations")

if(BLUSYS_BUILD_UI)
    # ui/widgets is included so products can drop in product-specific
    # widgets (files that aren't part of the platform widget kit) without
    # editing CMakeLists. The directory is scaffolded empty; sources only
    # appear if the product team adds them.
    list(APPEND src_dirs "ui" "ui/screens" "ui/widgets")
endif()

idf_component_register(
    SRC_DIRS ${src_dirs}
    INCLUDE_DIRS "."
    REQUIRES blusys blusys_services blusys_framework
)
```

Note on include style: because `INCLUDE_DIRS "."` makes the component include root the `app/` directory itself, files *inside* the app component use paths relative to `app/` — for example `#include "controllers/home_controller.hpp"`, not `#include "app/controllers/home_controller.hpp"`. Files *outside* the app component (in `main/app_main.cpp`, for instance) still use the same relative form because `main/` declares `REQUIRES app` and inherits the same include root.

**`main/CMakeLists.txt` template:**

```cmake
idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES app blusys_framework
)
```

**`app/product_config.cmake` template** (scaffold pre-populates based on `--starter` flag):

```cmake
set(BLUSYS_PRODUCT_NAME  "my_product")
set(BLUSYS_STARTER_TYPE  "interactive")   # or "headless"
set(BLUSYS_TARGETS       "esp32s3")
```

**`main/idf_component.yml` template** — pins all three platform components to the current release tag from `git@github.com:oguzkaganozt/blusys.git`. If `interactive`, also pins `lvgl/lvgl` at the platform-supported version.

**Other Phase 7 tasks:**
- **Remove `#include "blusys/blusys_all.h"` from the CLI scaffold template** — the header is removed in Phase 8 and the new scaffold must not reference it. Use `blusys/framework/framework.hpp` or `blusys/blusys_services.h` as needed.
- **Do not set `EXTRA_COMPONENT_DIRS = "$ENV{BLUSYS_PATH}/components"`** in the scaffolded CMakeLists. That is the pattern used by the monorepo's internal `examples/` (where the platform lives next door). Product repos consume the platform exclusively through managed components so the pinned tag is authoritative.
- Sample interactive product: counter app with a title, a value, and two buttons. ~50 lines of product code total.
- Sample headless product: blinking LED over MQTT. ~30 lines.

**Deliverables:**
- `blusys create my_product --starter interactive` produces a buildable, runnable project.
- Both sample apps boot to hardware using **only** managed-component dependencies (no local platform checkout required).
- The three CMakeLists.txt files (top-level, `main/`, `app/`) plus `app/product_config.cmake` are all scaffold-generated and commit-ready.
- The scaffold behavior is intentionally one-time: generated files are ready to edit in the product repo and are not part of a live sync/regenerate system.
- Headless scaffolds still route through `blusys_framework/core` and `main/app_main.cpp` by design; only `blusys_framework/ui` is excluded.

**Done when:**
- A new product can be scaffolded and reach first-boot on hardware without the developer touching LVGL, CMake, or low-level wiring directly.
- The chosen starter type (`headless` or `interactive`) produces the correct initial scaffold shape, and the `BLUSYS_BUILD_UI` gate works end-to-end for that scaffold. Changing starter type later is explicitly treated as a re-scaffold or manual-edit task in V1.
- A scaffolded product builds on a fresh developer machine that has **never** cloned the blusys repo — managed components pull the platform from its canonical git URL.

### Phase 8 — Example ecosystem migration

**Goal:** Migrate the existing example set to the new platform model.

**Work:**
- **Update example `REQUIRES` lists:**
  - HAL examples: `REQUIRES blusys` (unchanged — ~35 examples)
  - Driver examples: `REQUIRES blusys` (was `blusys_services` — 9 examples, already touched in Phase 3)
  - Service examples: `REQUIRES blusys_services` (still valid — ~20 examples, including `usb_hid_basic`)
  - Framework examples: `REQUIRES blusys_framework` (new — added alongside widget kit)
- **HAL, driver, and service examples stay `.c`** (services is C in V1 per decision 1). Only framework examples are `.cpp`.
- **Remove usages of `blusys_all.h`** and switch to per-tier umbrella headers. Full known call-site list (15 files, 17 total occurrences):
  - **Examples (2 files):**
    - `examples/ui_basic/main/ui_basic_main.c` — switch to `blusys/blusys_services.h` + `blusys/blusys.h`
    - `examples/lcd_st7735_basic/main/lcd_st7735_basic_main.c` — switch to `blusys/blusys.h`
  - **Guides (12 files, 14 occurrences):**
    - `docs/guides/ota-basic.md:31` — switch to `blusys/blusys_services.h`
    - `docs/guides/button-basic.md:19` — switch to `blusys/drivers/input/button.h`
    - `docs/guides/http-server-basic.md:22` — switch to `blusys/blusys_services.h`
    - `docs/guides/console-basic.md:16` — switch to `blusys/blusys_services.h`
    - `docs/guides/ws-client-basic.md:22` — switch to `blusys/blusys_services.h`
    - `docs/guides/mqtt-basic.md:21` — switch to `blusys/blusys_services.h`
    - `docs/guides/lcd-basic.md` lines 15, 72, 125 — switch to `blusys/drivers/display/lcd.h` (three occurrences)
    - `docs/guides/fatfs-basic.md:16` — switch to `blusys/blusys_services.h`
    - `docs/guides/fs-basic.md:16` — switch to `blusys/blusys_services.h`
    - `docs/guides/http-basic.md:17` — switch to `blusys/blusys_services.h`
    - `docs/guides/ui-basic.md:16` — switch to `blusys/blusys_services.h` + `blusys/blusys.h`
    - `docs/guides/wifi-prov-basic.md:18` — switch to `blusys/blusys_services.h`
  - **Scaffold template (1):** the `blusys` CLI `cmd_create` heredoc that generates `main/app_main.cpp` — already handled in Phase 7, verify nothing leaks into Phase 8.
- **Delete `components/blusys_services/include/blusys/blusys_all.h`** once no examples, guides, or scaffold templates reference it. (The file lives in services, not HAL — note the path.)
- **Verify `components/blusys/include/blusys/blusys.h`** now includes driver headers under `blusys/drivers/...` since drivers moved in Phase 3.
- **Add framework examples** (at least 3) showing widget kit usage:
  - A screen with layout primitives only (labels + rows + cols).
  - A screen with interactive widgets (button + slider + toggle).
  - An encoder-driven focus-traversal example using the input helpers.

**Done when:**
- All examples build on all supported targets (esp32, esp32c3, esp32s3).
- `blusys_all.h` is deleted from the repo; no file references it.
- Example ecosystem contains at least 3 framework-based examples.

### Phase 9 — Validation and migration notes

**Goal:** Formally validate the platform and write migration notes for future upgrades.

**Validation chain:**

```text
PC + LVGL + SDL2  →  ESP-IDF QEMU  →  real hardware
```

**Work:**
- Run the sample interactive product through all three validation stages.
- Run the sample headless product through stages 2 and 3.
- Capture first-boot success for both reference products on real hardware.
- Measure input → feedback latency on real hardware for the interactive product.
- Record RAM/flash deltas vs. the pre-migration baseline (report them, do not gate on fixed thresholds in V1).
- Write `docs/migration-guide.md` describing:
  - How to move an existing product from the old two-component model to the three-tier model
  - How to upgrade a product's pinned platform version in `idf_component.yml`
  - How to add a new capability to an existing product

**Done when:**
- At least one reference interactive product runs end-to-end on the platform.
- At least one reference headless product boots and exercises its primary flow on the platform.
- Validation notes capture the interactive-product latency measurement and RAM/flash deltas.
- The migration guide exists and is accurate.

## Parallel themes

These topics are tracked continuously across phases:

### 1. C++ discipline (framework only)
- No exceptions, no RTTI, no RAII over external handles.
- Constructors do no real work.
- `init/handle/tick/deinit` semantics preserved for stateful modules.
- No function-local statics with non-trivial destructors.
- Applies to framework code only; HAL, drivers, and services are C in V1.

### 2. UI thread safety
- No blocking calls under `blusys_ui_lock`.
- Service callbacks never mutate widgets directly — they post events to controllers.
- Controllers own all state transitions.

### 3. Allocation discipline
- Fixed-capacity containers + caller-owned buffers.
- No dynamic allocation in steady state.
- No `new`/`delete`, no `std::vector`/`std::string` members in framework code.

### 4. Modularity
- Starter-type-only product configuration in V1. Per-module gating deferred to V2.
- UI is an optional subsystem.
- Product-specific capabilities stay in product repos.

### 5. Layering enforcement
- CI lint check enforces HAL/drivers boundary inside `components/blusys/` (see [`DECISIONS.md`](DECISIONS.md) decision 16).
- Component boundaries enforced by ESP-IDF `REQUIRES`.
- Service-tier internal-helper usage stays pragmatic in V1 and is reviewed, not linted.
- No reverse dependencies allowed.

### 6. Public surface
- Include namespace `blusys/...` preserved.
- Per-tier umbrella headers: `blusys/blusys.h` (HAL+drivers, C), `blusys/blusys_services.h` (services, C), `blusys/framework/framework.hpp` (framework, C++).
- `blusys/blusys_all.h` removed at end of transition.

### 7. UX direction
- Physical-control focused product experiences.
- Fast tactile feedback (Teenage Engineering-style feel).
- Every interaction has at least one instant feedback channel on interactive products.

### 8. Router contract
- Minimum command set: `set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`.
- Feedback is a separate channel from router commands.
- Headless products do not emit router commands by default.

### 9. Framework gating
- `blusys_framework/core` is always available.
- `blusys_framework/ui` is only built when `BLUSYS_BUILD_UI` CACHE variable is `ON`, derived from `BLUSYS_STARTER_TYPE=interactive` in the product's `product_config.cmake`.

## Risks

- **Repo language changing before packaging shape is decided.** Mitigated by narrowing Phase 2 to identity-only and moving packaging prose to Phase 3.
- **Driver convention drift inside `components/blusys/`.** Mitigated by directory discipline (`src/common/` vs `src/drivers/`) plus the CI lint check added in Phase 3. If the check fails to catch real drift in practice, the drivers split can be retrofitted — file moves only, no API changes.
- **`blusys_framework` scope growing too early.** Mitigated by the V1 scope being locked to minimal spine + widget kit; everything else explicitly deferred to V2.
- **Scaffolding and migration notes lagging behind implementation.** Mitigated by Phases 7 and 9 each having concrete deliverables tied to a reference product.
- **First widget taking longer than expected.** Mitigated by Phase 5a budgeting explicitly for the flagship widget as a pattern-establishing exercise, and by Phase 4.5 isolating the SDL2 harness bring-up.
- **SDL2 harness takes longer than expected.** Mitigated by Phase 4.5 being its own explicit phase. If the harness slips, Phase 5 can fall back to QEMU-only validation temporarily.
- **LVGL version churn.** Mitigated by pinning the LVGL version in `idf_component.yml` and upgrading deliberately.

## Recommended sequence

1. Phase 1 — lock decisions (this phase)
2. Phase 2 — identity alignment (narrow)
3. Phase 3 — packaging shape (drivers into `blusys/`, framework stub, CI lint)
4. Phase 4 — C++ infrastructure (framework only, containers, `blusys/log.h`)
5. Phase 4.5 — PC + SDL2 host harness
6. Phase 5 — flagship widget and V1 widget kit (sub-phased)
7. Phase 6 — framework core V1 (in parallel with Phase 5)
8. Phase 7 — product scaffold and sample apps
9. Phase 8 — example ecosystem migration
10. Phase 9 — validation and migration notes

## Success criteria

The transition is successful when:

- The repo's identity is "platform," not "library," in docs and code.
- Product teams can scaffold a new product and reach first-boot on hardware without touching LVGL, CMake, or low-level wiring directly.
- At least one reference interactive product runs end-to-end on the platform.
- At least one reference headless product boots and exercises its primary flow on the platform.
- HAL, drivers, and services stay C. Framework is C++. (Services migration to C++ is a V2 concern.)
- The widget kit produces visually-consistent screens across products through the `theme_tokens` struct and the component contract.
- New products consume the platform via `idf_component.yml` with semver pinning.
- Validation notes record the reference-product latency measurement and RAM/flash deltas.

This list stays intentionally light. Blusys is an internal tool, so we avoid hard numeric gates, but the small evidence set above keeps the transition concrete and trustworthy.
