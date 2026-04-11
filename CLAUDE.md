# CLAUDE.md

Guidance for working in this repository during the **product-platform reset** (v7 path). When in doubt, trust executable sources over prose.

## Read first

Before changing public API, docs, repo structure, examples, scaffolding, or project status, read:

| File | Purpose |
|------|---------|
| `PRD.md` | Product requirements for the reset |
| `docs/internals/architecture.md` | Architecture and tiering |
| `docs/internals/guidelines.md` | Public API rules and contribution workflow |
| `inventory.yml` | Module, example, and doc manifest |

Authoritative when behavior matters: the `blusys` shell script, `components/*/CMakeLists.txt`, example `idf_component.yml` files, and `.github/workflows/*.yml`.

## Repository mission

**Single active mission:** ship the product-platform reset.

That reset is a breaking redefinition of the product-facing surface: smaller app code, more logic in the framework, low-level escape hatches for advanced use, less repo-wide churn.

This is **not** aiming to be a cleaner generic ESP32 framework. It is the internal OS for recurring product families. Prefer reusable product flows over module sprawl.

## Locked decisions

Treat as constraints unless the user explicitly changes them:

- Product-facing namespace: `blusys::app` (C++ only on the product path)
- App model: reducer-style `update(ctx, state, action)`; reducers mutate state in place
- Core runtime modes: **`interactive`** and **`headless`** only
- Default onboarding: **host-first** interactive; secondary path: **headless-first** hardware
- “Consumer” / “industrial” are **product lenses**, not framework branches
- Four archetypes: interactive controller, interactive panel, edge node, gateway/controller
- First canonical interactive hardware profile: **generic SPI ST7735** (ESP32, ESP32-C3, ESP32-S3)
- Product-facing term: **`capabilities`**, not “bundles”
- Hardware and capability configuration: **code-first**; Kconfig is advanced tuning
- Raw LVGL: only inside custom widgets or an explicit custom view scope; outward UI behavior goes through **actions** and approved framework behavior
- Scaffold: single local component **`main/`** with fixed layout **`core/`**, **`ui/`**, **`integration/`** (behavior · rendering · wiring and capability calls)
- Three-tier architecture stays; HAL, services, `blusys_ui`, and low-level framework primitives remain **escape hatches**

## Architecture constraints

| Component | Role |
|-----------|------|
| `components/blusys/` | HAL + drivers (C) |
| `components/blusys_services/` | Runtime substrate (C) |
| `components/blusys_framework/` | Product framework (C++) |

```text
blusys_framework -> blusys_services -> blusys
```

Do not collapse tiers as part of this refactor.

## Refactor priorities

1. Shrink and simplify the product-facing app path  
2. Remove framework/UI lifecycle plumbing from normal app code  
3. Make host-first interactive onboarding runnable by default  
4. Build headless on the same app runtime model  
5. Reusable operating flows instead of one-off wiring  
6. Framework-owned defaults and capabilities instead of ad hoc orchestration  
7. Trim docs, examples, and CI once the new path is real  

## Archetypes

Ground work in the four canonical archetypes (interactive controller, interactive panel, edge node, gateway/controller). Judge changes by whether they make those shapes easier on **one** shared operating model.

## Product model target

**App owns:** `state`, `action`, `update(ctx, state, action)`, screens/views, optional hardware profiles, optional capabilities.

**Framework owns:** boot/shutdown, runtime loop and tick, routing, feedback, LVGL lifecycle and locks, screen activation/overlays, host/device/headless adapters, input bridges, default runtime-service orchestration, reusable product flows.

**Normal product code should not touch directly:** `runtime.init`, `route_sink`, `feedback_sink`, `blusys_ui_lock`, `lv_screen_load`, raw LCD bring-up on the canonical interactive path, raw Wi-Fi orchestration on the canonical connected path.

## Scaffold shape

```text
my_project/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── core/
    ├── ui/
    └── integration/
```

Do not introduce competing default layouts per archetype.

## UI rules (v7)

**Preferred layers:** (1) stock `blusys::app` widgets, (2) product widgets following the widget contract, (3) bounded raw LVGL for advanced rendering.

**Product widgets:** lightweight contract — `config`/`props`, semantic callbacks or `dispatch(action)`, theme tokens only for visuals, setters own transitions where needed, standard focus/disabled behavior, raw LVGL inside the implementation only.

**Raw LVGL must not:** own app screens, take UI locks, call runtime services from UI, or manipulate routing/runtime internals.

## Code direction

- Smallest change toward the v7 target  
- Delete boilerplate; do not wrap it  
- Keep escape hatches available but clearly separated  
- No compatibility shims unless migration truly needs them  
- No generic abstractions that do not reduce real app burden  

## CLI reference

Normal build and validation go through **`blusys`**.

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
blusys example <name> [command] [args...]   # e.g. example quickstart/interactive_controller run /dev/ttyACM0 esp32s3
blusys qemu           [project] [esp32|esp32c3|esp32s3]
blusys install-qemu
blusys lint
blusys config-idf
blusys version
blusys update
```

Commands reflect **current** repo behavior; scaffolding may still simplify over time.

## Validation

Proportionate checks:

1. `blusys lint` — layering-sensitive changes  
2. `blusys build` or `blusys host-build` — affected paths  
3. `mkdocs build --strict` — docs/nav  
4. `python scripts/check-inventory.py` — examples, docs, or modules added/removed  
5. `python scripts/check-product-layout.py` — `main/core` · `main/ui` · `main/integration` for product-shaped examples  
6. `python scripts/example-health.py` — example inventory paths  
7. Broader nightly validation — only when warranted  

PR CI builds examples with `ci_pr=true` in `inventory.yml` (archetype quickstarts). Full examples run nightly.

After public app-flow changes, prioritize: host-first interactive, headless, scaffolded product flow, ST7735 builds on all three targets.

## Examples and docs

**Examples:** `quickstart/` (PR CI), `reference/` (nightly), `validation/` (internal, nightly). New examples need `inventory.yml` entries (category, visibility, CI flags).

**Docs IA:** Start → App → Services → HAL + Drivers → Internals. New pages: `inventory.yml` + `mkdocs.yml`. Prefer canonical quickstarts and task-oriented pages; keep advanced material off the main path; one combined guide + API page per module where practical.

## Module work

**HAL / services (C):** public APIs return `blusys_err_t`; public headers stay free of ESP-IDF types; preserve SOC gating and optional managed-component patterns in CMake.

**Framework (C++):** no exceptions, no RTTI, no RAII over LVGL or ESP-IDF handles; avoid steady-state heap unless by deliberate design.

## Do not

- Collapse the three tiers  
- Move runtime services to C++ unless directed  
- Build a heavy compatibility layer for the old product path  
- Let raw LVGL become the normal path again  
- Let UI call runtime services directly in the new model  
- Keep old and new public app models in parallel longer than necessary  

## Generated files

Ignore `examples/**/build*`, `examples/**/sdkconfig*` (except `sdkconfig.defaults*`), and `site/` unless the task is about generated output. Do not edit vendored `esp-idf-en-*` unless the task is about that copy.
