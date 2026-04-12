# CLAUDE.md

Guidance for working in **Blusys** — the internal ESP32 product platform. When behavior matters, trust **executable sources** (CMake, `blusys`, workflows, `inventory.yml`) over this file.

**Package version:** `6.1.1` (`BLUSYS_VERSION_STRING` in `components/blusys_hal/include/blusys/version.h`; also `blusys version`).  
**ESP-IDF:** **v5.5+** (CI uses the `espressif/idf` container pinned in `.github/workflows/*.yml`).

---

## Read first

Before changing public API, docs, repo structure, examples, scaffolding, or project status:

| File | Purpose |
|------|---------|
| `README.md` | Product mission, locked decisions, scaffold rules (**Product foundations**) |
| `PRD.md` | Stable pointer to the same scope as README **Product foundations** |
| `docs/internals/architecture.md` | Three-tier model, module layout, layering rules |
| `docs/internals/guidelines.md` | Public API rules, product app shape, workflow |
| `inventory.yml` | Modules, examples, docs — **CI and manifest source of truth** |
| `mkdocs.yml` | Published doc navigation (must stay aligned with `inventory.yml`) |
| `docs/app/validation-host-loop.md` | Maps validation goals to `scripts/host/`, scaffold smoke, CI |

**Authoritative for build behavior:** repo-root `blusys`, `components/*/CMakeLists.txt`, `cmake/blusys_framework_ui_sources.cmake`, `cmake/blusys_framework_host_widgetkit.cmake`, example `main/idf_component.yml`, `.github/workflows/*.yml`.

---

## What this repository is

Blusys is **not** a generic ESP-IDF tutorial stack. It is a shared **operating model** for recurring product families: fixed `main/` layout (`core/` · `ui/` · `integration/`), reducer-driven `update(ctx, state, action)`, **capabilities** for cross-cutting runtime concerns, **host-first** interactive iteration (SDL2 + LVGL) where useful, and **escape hatches** to HAL/services for advanced use.

**Non-goal:** collapsing the three tiers or turning services into C++ on the product path.

---

## Locked decisions (product foundations)

Unless the user explicitly changes them:

- **Product API:** `blusys::app` (C++ on the product path).
- **App logic:** reducer `update(ctx, state, action)` with **in-place** state mutation.
- **Runtime modes:** **`interactive`** and **`headless`** only.
- **Onboarding defaults:** **host-first** interactive; **headless-first** hardware as the secondary path.
- **B2C vs B2B:** product **lenses**, not separate framework runtimes.
- **Archetypes (starters):** interactive controller, interactive panel, edge node, gateway/controller.
- **First canonical interactive hardware profile:** generic SPI **ST7735** (ESP32, ESP32-C3, ESP32-S3).
- **Terminology:** **`capabilities`**, not “bundles”.
- **Configuration:** **code-first** hardware/capability wiring; Kconfig for advanced tuning.
- **LVGL:** raw LVGL only inside custom widgets or an explicit custom view scope; app-facing UI drives behavior through **actions** and framework-approved paths.
- **Scaffold:** one local `main/` component with **`core/`** (behavior), **`ui/`** (screens/views; may be minimal for headless), **`integration/`** (thin wiring, `app_spec`, entry).
- **Tiers:** `blusys_framework` → `blusys_services` → `blusys_hal` — **no reverse dependencies**; HAL, services, `blusys_ui`, and low-level primitives remain **escape hatches**, not the default product path.

---

## Architecture

| Component | Role |
|-----------|------|
| `components/blusys_hal/` | HAL + drivers (C); `blusys_err_t` public API |
| `components/blusys_services/` | Runtime substrate (C): connectivity, UI service, protocols, FS, etc. |
| `components/blusys_framework/` | Product framework (C++): `blusys::app`, views, routing, widget kit |

```text
blusys_framework → blusys_services → blusys_hal → ESP-IDF
```

**Layering:** enforced by `blusys lint` (`scripts/lint-layering.sh`) plus `python3 scripts/check-framework-ui-sources.py` (framework UI source lists / widget glob consistency).

---

## Product model (ownership)

**App owns:** `state`, `action`, `update`, screens/views, optional profiles, optional capabilities.

**Framework owns:** boot/shutdown, runtime loop/tick, routing, feedback, LVGL lifecycle and locks, screen activation/overlays, host/device/headless adapters, input bridges, default service orchestration, reusable flows.

**Normal product code should not depend on:** `runtime.init`, `route_sink`, `feedback_sink`, `blusys_ui_lock`, `lv_screen_load`, raw LCD bring-up on the canonical interactive path, or raw Wi-Fi orchestration on the canonical connected path — those stay framework-owned.

---

## Scaffold and project creation

- **`blusys create`** copies from **canonical examples** (e.g. `examples/quickstart/interactive_controller`, `examples/quickstart/edge_node`, `examples/reference/gateway`) — see `scripts/lib/blusys/create.sh`.
- Generated projects get a top-level `CMakeLists.txt`, `sdkconfig.defaults`, `main/` tree, and **`host/CMakeLists.txt`** where the archetype supports host builds.
- **Gateway headless** may overlay files from `templates/scaffold/gateway_headless/` after the example copy.
- **`scripts/scaffold-smoke.sh`** exercises create × archetypes + `blusys host-build` (also runs in CI).

Do **not** introduce competing default `main/` layouts per archetype.

---

## Host harness (`scripts/host/`)

- **Purpose:** LVGL **v9.2.x** (FetchContent, pinned to match the managed component) on **SDL2** for fast UI iteration without flashing.
- **Entry:** `blusys host-build` with optional project path; no path configures/builds `scripts/host`.
- **Framework bridge:** static library compiling `blusys_framework` UI + spine sources listed via `cmake/blusys_framework_ui_sources.cmake` (widget `.cpp` files under `src/ui/widgets/*/*.cpp` are **glob-discovered**; do not hand-list new widgets in multiple places — run `python3 scripts/check-framework-ui-sources.py` after CMake edits).
- **Platform glue:** `scripts/host/src/app_host_platform.cpp` (interactive), `app_headless_platform.cpp` (headless).
- **Encoder on host:** arrow keys + Enter map to framework intents (see platform source).
- **Tests:** `enable_testing()`; **`host_spine_test`** (containers/spine without full LVGL stack dependency for ring buffer checks), **`snapshot_buffer_smoke`** (stub display + partial flush, headless framebuffer path). Run **`ctest`** from the host build dir.
- **Sanitizers:** configure with **`-DBLUSYS_HOST_SANITIZE=ON`** for ASan/UBSan on selected targets (see CI “Host spine tests (sanitizer)” job).
- **Deps:** `libsdl2-dev`, `pkg-config`, `cmake` — see `scripts/host/README.md`.

Headless host builds that compile `integration/app_main.cpp` with **`build_profile.hpp`** need **`components/blusys_services/include`** on the include path (see `examples/quickstart/edge_node/host/CMakeLists.txt`) so `blusys/display/ui.h` resolves.

---

## Widget kit

- **Contract:** `components/blusys_framework/widget-author-guide.md` — six-rule contract; reference **`bu_button`**.
- **Camp 2:** fixed slot pools for many stock widgets (`BLUSYS_UI_<WIDGET>_POOL_SIZE`).
- **Camp 3 (example):** `bu_knob` uses **per-instance** data via LVGL’s allocator (`lv_malloc` / free on delete) instead of a global pool — pattern for new widgets that should not be pool-sized in Kconfig.

Stock widgets are included in device and host builds through **`cmake/blusys_framework_ui_sources.cmake`** (shared with ESP-IDF `components/blusys_framework/CMakeLists.txt`).

---

## CI (PR) — condensed

Workflow: **`.github/workflows/ci.yml`** (push/PR to `main` on listed paths).

Typical stages:

1. **Lint:** `scripts/lint-layering.sh` + `scripts/check-framework-ui-sources.py`
2. **Inventory:** `scripts/check-inventory.py` + `scripts/check-product-layout.py`
3. **Host smoke:** configure/build `scripts/host` (including spine + snapshot smokes), run smoke binaries, **`ctest`**, sanitizer build/run for spine tests; build **edge_node** + **gateway** host examples; **`scripts/scaffold-smoke.sh`**
4. **Docs:** `mkdocs build --strict`
5. **Device builds:** `scripts/build-from-inventory.sh` with `ci_pr=true` per target matrix
6. **Display variants:** `scripts/build-display-variants.sh`
7. **QEMU smokes:** subset via `scripts/qemu-test.sh`

**Nightly:** `.github/workflows/nightly.yml` — broader example builds.

PR examples are those with **`ci_pr: true`** in `inventory.yml`.

---

## Validation (local)

Use proportionate checks:

| Check | When |
|-------|------|
| `blusys lint` | Layering or framework UI CMake / widget paths |
| `blusys build` / `blusys host-build` | Anything touching components or examples |
| `mkdocs build --strict` | Docs or `mkdocs.yml` |
| `python3 scripts/check-inventory.py` | `inventory.yml`, new modules/examples/docs |
| `python3 scripts/check-product-layout.py` | Product-shaped `main/` layouts |
| `python3 scripts/check-framework-ui-sources.py` | `cmake/blusys_framework_ui_sources.cmake` |
| `python3 scripts/example-health.py` | Example paths vs inventory |
| `scripts/scaffold-smoke.sh` | `blusys create`, templates, host CMake for archetypes |

After app-flow or integration changes, prioritize: **host interactive**, **headless**, **scaffold**, and **ST7735-class** device builds on **esp32 / esp32c3 / esp32s3**.

---

## Code direction

- Smallest change that **advances the canonical product path**
- Delete boilerplate; avoid wrapping it in new abstraction layers
- Keep escape hatches visible and documented
- No compatibility shims unless a migration truly needs them
- No generic abstractions that do not reduce real app burden

---

## UI rules (summary)

**Preferred layers:** (1) stock `blusys::app` widgets and views, (2) product widgets following the widget contract, (3) bounded raw LVGL inside custom widgets / custom view scope only.

**Raw LVGL must not:** own app screens, take UI locks, call runtime services from UI, or manipulate routing/runtime internals.

---

## CLI reference

Normal workflows use the repo-root **`blusys`** launcher:

```bash
blusys create [--archetype <name>] [--starter <headless|interactive>] [--list-archetypes] [path]
blusys build          [project] [esp32|esp32c3|esp32s3]
blusys flash          [project] [port] [esp32|esp32c3|esp32s3]
blusys monitor        [project] [port] [esp32|esp32c3|esp32s3]
blusys run            [project] [port] [esp32|esp32c3|esp32s3]
blusys config         [project] [esp32|esp32c3|esp32s3]
blusys menuconfig     [project] [esp32|esp32c3|esp32s3]
blusys size           [project] [esp32|esp32c3|esp32s3]
blusys erase          [project] [port] [esp32|esp32c3|esp32s3]
blusys clean          [project] [esp32|esp32c3|esp32s3]
blusys fullclean      [project]
blusys build-examples
blusys host-build     [project]
blusys example <name> [command] [args...]
blusys qemu           [project] [esp32|esp32c3|esp32s3]
blusys install-qemu
blusys lint
blusys config-idf
blusys version
blusys update
```

Implementation: `scripts/lib/blusys/*.sh` sourced from repo-root `blusys`.

---

## Module work

**HAL / services (C):** public APIs return `blusys_err_t`; public headers avoid ESP-IDF types; preserve SOC gating and optional managed-component patterns in CMake. **Migrating services to C++ is out of scope** — the services tier stays C.

**Framework (C++):** `-fno-exceptions -fno-rtti` (see `components/blusys_framework/CMakeLists.txt`); no RAII over LVGL or ESP-IDF handles; avoid steady-state heap except by deliberate design (widgets may allocate at create time per contract).

---

## Do not

- Collapse or reverse the three tiers
- Move runtime services to C++ unless explicitly directed
- Build a heavy “old vs new” compatibility layer for obsolete product paths
- Let raw LVGL become the default product path again
- Let UI call runtime services directly in the product model
- Edit generated trees under `examples/**/build*`, `site/`, or vendored IDF language packs (`esp-idf-en-*`) unless the task is about those artifacts

---

## Generated / ignored paths

Ignore `examples/**/build*`, `examples/**/sdkconfig*` (except `sdkconfig.defaults*`), and `site/` unless the task concerns generated output.
