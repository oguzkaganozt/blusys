# CLAUDE.md

This file provides guidance for working in this repository during the current product-platform reset.

## Read First

Before changing public API, docs, repo structure, examples, scaffolding, or project status, read these files first:

- `PRD.md` — canonical product requirements for the current platform reset
- `ROADMAP.md` — canonical execution roadmap for the current platform reset
- `docs/internals/architecture.md` — current code architecture and tiering
- `docs/internals/guidelines.md` — public API design rules and contribution workflow
- `inventory.yml` — module, example, and doc classification manifest

Trust executable sources over prose when they disagree: the `blusys` shell script, `components/*/CMakeLists.txt`, example `idf_component.yml` files, and `.github/workflows/*.yml` are authoritative about current behavior.

## Repository Mission

This repository has one active mission: execute the current product-platform reset.

The reset is a breaking redefinition of the product-facing surface. The goal is to make product code dramatically simpler, move complexity into the framework, preserve low-level escape hatches as advanced paths, and reduce repo-wide maintenance burden.

This repository is not trying to become a cleaner generic ESP32 framework. It is trying to become the internal operating system for our recurring product families.

When making decisions, favor reusable product operating flows over generic module sprawl.

## Locked Decisions

These are already decided and should be treated as constraints unless explicitly changed by the user:

- Product-facing namespace is `blusys::app`
- Product-facing path is C++-only
- App logic model is reducer-style: `update(ctx, state, action)`
- Reducers mutate state in place
- Default onboarding is host-first interactive
- Secondary canonical path is headless-first hardware
- The only core runtime modes are `interactive` and `headless`
- Consumer and industrial are product lenses, not framework branches
- The four canonical archetypes are `interactive controller`, `interactive panel`, `edge node`, and `gateway/controller`
- First canonical interactive hardware profile is generic SPI ST7735
- ST7735 profile should support ESP32, ESP32-C3, and ESP32-S3
- Product-facing term is `capabilities`, not `bundles`
- Hardware and capability configuration for the product path is code-first; Kconfig is advanced tuning only
- Raw LVGL is allowed only inside custom widget implementations or explicit custom view scope
- UI outward behavior must go only through actions and approved framework behavior
- Scaffolded product apps use `main/` as the single local ESP-IDF component
- Scaffolded product apps use the fixed internal structure `core/`, `ui/`, and `integration/`
- `core/` owns product behavior, `ui/` owns rendering and widgets, `integration/` owns wiring and runtime-service integration
- Capability work is requested by reducer-driven actions and translated into capability calls in `integration/`
- The three-tier architecture stays in place
- HAL, runtime services, `blusys_ui`, and low-level framework primitives remain available as advanced escape hatches

## Architecture Constraints

The three-tier structure remains the base architecture:

- `components/blusys/` — HAL + drivers (C)
- `components/blusys_services/` — runtime substrate (C)
- `components/blusys_framework/` — product framework (C++)

Dependency direction remains:

```text
blusys_framework -> blusys_services -> blusys
```

Do not collapse tiers as part of this refactor.

## Refactor Priorities

When deciding what to work on, prioritize these outcomes:

1. Make the product-facing app path smaller and simpler.
2. Remove framework and UI lifecycle plumbing from normal app code.
3. Make host-first interactive onboarding actually runnable by default.
4. Build the headless path on the same app runtime model.
5. Build reusable product operating flows instead of one-off app wiring.
6. Replace low-level product orchestration with framework-owned defaults and capabilities.
7. Shrink docs, examples, and CI burden after the new path is real.

## Archetypes

The reset should stay grounded in the four canonical archetypes:

- interactive controller
- interactive panel
- edge node
- gateway/controller

Work should be evaluated against whether it makes those archetypes easier to build on one shared operating model.

## Product Model Target

The intended v7 product path centers on these app-owned concepts:

- `state`
- `action`
- `update(ctx, state, action)`
- screens and views
- optional hardware profiles
- optional capabilities

The framework should own:

- app boot and shutdown
- runtime loop and tick cadence
- navigation and routing ownership
- feedback plumbing and defaults
- LVGL lifecycle and lock discipline
- screen activation and overlays
- host, device, and headless adapters
- common input bridges
- common runtime-service orchestration for the default path
- reusable operating flows for common product behaviors

Normal product code should not need to touch these concepts directly:

- `runtime.init`
- `route_sink`
- `feedback_sink`
- `blusys_ui_lock`
- `lv_screen_load`
- raw LCD bring-up in the canonical interactive path
- raw Wi-Fi lifecycle orchestration in the canonical connected path

## Scaffold Shape

The canonical scaffold shape for product apps is:

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

Do not introduce competing default project layouts for different archetypes.

## UI Rules For v7 Work

The framework should be strongly opinionated, but custom UI composition must remain possible inside defined boundaries.

Preferred UI layers:

1. Stock `blusys::app` widgets and helpers
2. Product-owned widgets that follow the Blusys widget contract
3. Explicit bounded raw LVGL blocks for advanced rendering

Do not introduce a heavy inheritance tree for user widgets.

Instead, product-owned widgets should follow a lightweight contract:

- public `config` or `props` struct
- semantic callbacks or direct `dispatch(action)` only
- theme tokens as the only visual source
- setters own state transitions when needed
- standard focus and disabled behavior for interactive widgets
- raw LVGL stays inside the widget implementation

Raw LVGL is allowed only in approved custom scopes. It must not:

- manage app screens directly
- manage UI locks directly
- call runtime services directly from UI code
- manipulate routing or runtime internals directly

## Code Direction

Follow these implementation preferences during the refactor:

- prefer the smallest change that moves the codebase toward the v7 target
- remove boilerplate rather than wrapping it in more boilerplate
- keep low-level escape hatches available, but clearly separate them from the recommended path
- avoid compatibility shims unless they are strictly needed for staged migration
- avoid introducing generic abstractions that do not remove real app-code burden

## Current Commands

All normal build and validation commands still go through the `blusys` CLI.

```bash
blusys create [--starter <headless|interactive>] [path]
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
blusys example <name> [build|flash|monitor|run] [port] [target]
blusys qemu           [project] [esp32|esp32c3|esp32s3]
blusys install-qemu
blusys lint
blusys config-idf
blusys version
blusys update
```

These commands reflect current repository behavior, not the final v7 shape. Part of the refactor is to simplify scaffolding and the product creation flow.

## Validation Guidance

Use these checks proportionally to the change:

1. `blusys lint` for layering-sensitive work
2. targeted `blusys build` or `blusys host-build` for the affected path
3. `mkdocs build --strict` for docs or nav changes
4. `python scripts/check-inventory.py` when adding or removing examples, docs, or modules
5. `python scripts/example-health.py` when modifying example structure
6. broader nightly validation only when the touched area warrants it

PR CI only builds examples marked `ci_pr=true` in `inventory.yml` (currently the archetype quickstarts). Full example validation runs nightly.

When the refactor changes public app flow, prioritize validating:

- host-first interactive path
- headless path
- scaffolded product flow
- ST7735 compile support across all three targets

## Examples And Docs Guidance

Examples are organized into three categories under `examples/`:

- `quickstart/` — canonical product starters using `blusys::app` (public, PR CI)
- `reference/` — scoped capability demos for one module each (public, nightly CI)
- `validation/` — internal stress and smoke tests (internal, nightly CI)

When adding new examples, add an entry to `inventory.yml` with the correct category, visibility, and CI flags.

Docs follow the IA: `Start` → `App` → `Services` → `HAL + Drivers` → `Internals` → `Archive`.

When adding or refactoring docs:

- favor canonical quickstarts over proliferating near-duplicate examples
- favor family- or task-oriented docs over page-per-everything sprawl
- keep advanced and validation-only material clearly separated from the main user path
- add new doc pages to both `inventory.yml` (docs section) and `mkdocs.yml` (nav)
- consolidate guide and API reference into a single page per module

## Module Work Guidance

If the task touches HAL, runtime services, or framework internals:

- keep return types as `blusys_err_t` in public C APIs
- keep public C headers free of ESP-IDF types and macros
- preserve the current SOC-gated module pattern where applicable
- preserve current optional managed-component detection patterns in CMake

If the task touches the framework:

- no exceptions
- no RTTI
- no RAII over LVGL or ESP-IDF handles
- avoid steady-state dynamic allocation unless a deliberate design change requires it

## What Not To Do

- do not collapse the three-tier architecture
- do not migrate runtime services to C++ as part of the reset unless explicitly directed
- do not build a heavy compatibility layer for the old product path
- do not let raw LVGL become the normal product path again
- do not let UI code call runtime services directly in the new model
- do not keep old and new public app models alive in parallel longer than necessary

## Generated Files

Ignore generated artifacts under `examples/**/build*`, `examples/**/sdkconfig*` (except `sdkconfig.defaults*`), and `site/` unless the task is specifically about generated output.

Do not edit vendored upstream docs under `esp-idf-en-*` unless the task is explicitly about that copy.
