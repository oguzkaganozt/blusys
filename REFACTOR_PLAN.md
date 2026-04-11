## Blusys Final Refactor Roadmap

This is the last refactor plan for the platform reset.
It is intentionally breaking and optimized for a simpler production system, not compatibility.

## Target Outcome

The repo should converge on one canonical product model:

- `blusys::app` is the only recommended product-facing path
- product apps use `main/core/`, `main/ui/`, and `main/integration/`
- `core/` owns state, actions, reducer logic, and product rules
- `ui/` owns screens, widgets, and rendering
- `integration/` owns entry, app spec, capabilities, profile selection, and event bridging
- `integration/` stays thin and boring
- `system/` and `logic/` are retired from the recommended scaffold

## Canonical Scaffold

Default scaffold shape:

```text
my_product/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── core/
    │   ├── app.hpp
    │   └── app.cpp
    ├── ui/
    │   └── app_ui.cpp
    └── integration/
        ├── app_main.cpp
```

Starter-specific rules:

- interactive starters may include a top-level `host/` directory
- headless starters do not need `host/`
- do not make `host/` part of the universal scaffold contract
- the default scaffold should stay as small as possible; split files only when a product actually needs the extra structure

## Ownership Rules

### `main/core/`

- owns state
- owns actions
- owns `update(ctx, state, action)`
- owns product rules and reducer transitions
- owns pure app decisions
- declares product-facing types and hooks used by `ui/` and `integration/`
- must not call LVGL directly
- must not call runtime services directly
- must not contain app entry or profile wiring

### `main/ui/`

- owns screens
- owns widgets
- owns layout and visual composition
- dispatches actions into `core`
- may use `blusys::app::view`
- must not call runtime services directly
- must not own startup or capability wiring

### `main/integration/`

- owns `app_spec`
- owns app identity, theme, and feedback selection
- owns capability instances and configs
- owns profile selection
- owns event-to-action bridging
- owns entry macro hookup
- owns host/device/headless differences
- must stay thin and contain no product behavior

## Refactor Principles

1. Keep one local ESP-IDF component per product app.
2. Rename `logic/` to `core/`.
3. Rename `system/` to `integration/`.
4. Keep `ui/` as the rendering boundary.
5. Move wiring out of product logic and into framework helpers whenever possible.
6. Remove old paths and old terminology instead of keeping them alive in parallel.
7. Keep the generated scaffold as small as practical.

## Current Problems To Remove

### Repo coherence drift

- scaffold, docs, examples, and CLI still describe different layouts
- stale `logic/` and `system/` terminology remains in public and generated output
- stale `app/product_config.cmake` references still exist
- examples mix canonical and legacy product shapes

### Product API leakage

- canonical examples still reach into framework internals directly
- `bundle` terminology still appears in public seams
- app identity and theme selection are not fully centralized
- event mapping is still too manual in some product starters

### Service complexity

- Wi-Fi and BLE families repeat bootstrap and ownership logic
- filesystem flows overlap in responsibility
- several service modules mix orchestration with transport details

## Roadmap

### Phase 1: Lock The New Scaffold

- standardize the product scaffold on `main/core/ui/integration`
- remove `logic/` and `system/` from the recommended path
- make `integration/` the only place for app wiring and startup glue
- update scaffold documentation to define what is forbidden in each folder
- keep the default scaffold file count minimal

### Phase 2: Replace Scaffold Generation

- replace heredoc scaffold generation in `blusys create`
- generate from checked-in templates or canonical example sources
- make the new scaffold the only generated output for product apps
- keep README generation part of the scaffold output
- keep `host/` starter-specific, not universal

### Phase 3: Seal The Public App Surface

- keep `blusys::app` as the only recommended product-facing API
- finish the `bundles` to `capabilities` cutover
- move identity, theme, and feedback selection into `integration/`
- remove legacy public seams that expose runtime plumbing unnecessarily
- keep raw framework APIs as advanced-only surfaces

### Phase 4: Simplify App Composition

- keep state, actions, and reducer logic in `core/`
- keep screens and widgets in `ui/`
- keep spec, profile choice, and capability instances in `integration/`
- keep `integration/` usually as a single wiring file unless growth justifies splitting it
- reduce large `map_event()` switchboards
- move common capability-to-action bridges into framework helpers
- make navigation, shell, and overlays more framework-owned and declarative

### Phase 5: Simplify Framework Internals

- unify focus and input ownership on one model
- reduce singleton-style UI plumbing
- keep screen registration declarative
- keep escape hatches available, but move them out of the normal path
- ensure normal product apps do not need raw LVGL lifecycle code

### Phase 6: Consolidate Services Internals

- add shared connectivity runtime for Wi-Fi-family services
- add shared BLE runtime for BLE-family services
- consolidate filesystem internals behind shared helpers
- split oversized modules by responsibility where needed
- preserve the three-tier architecture

### Phase 7: Curate Examples

- keep canonical quickstarts limited to the archetypes that define the product path
- align quickstarts with the new scaffold shape
- demote framework demos to reference or validation-only use
- archive duplicate or competing app-shape examples
- keep host-first interactive and headless flows runnable

### Phase 8: Align Docs, Inventory, And Tooling

- remove stale references to older scaffold models
- align docs, CLI help, inventory, and validation scripts with `core/ui/integration`
- make health checks enforce the canonical shape
- keep one current truth for the recommended app path
- keep generated examples and quickstarts aligned to the minimal scaffold, not a larger template

### Phase 9: Production-Readiness Validation

- validate the scaffolded host-first interactive path
- validate the scaffolded headless path
- validate the ST7735 interactive compile path on `esp32`, `esp32c3`, and `esp32s3`
- add runtime smoke validation for navigation, input, and capabilities
- keep PR CI focused and nightly broader

## Execution Order

1. rename the scaffold to `core/ui/integration`
2. replace `blusys create` output with the new scaffold
3. migrate canonical quickstarts to the new layout
4. remove docs, CLI, and inventory drift
5. simplify framework app composition helpers
6. consolidate service internals
7. tighten CI around the canonical path

## Non-Goals

- collapsing the three-tier architecture
- rewriting services in C++
- preserving old scaffold shapes for compatibility
- keeping multiple public product paths alive longer than necessary

## Success Criteria

The refactor is done when:

- a new product app starts from `main/core/ui/integration`
- `integration/` contains only wiring, not product behavior
- the default scaffold stays compact instead of growing boilerplate back
- product code does not manage runtime plumbing directly
- docs, examples, scaffold, and CLI all describe the same app model
- the canonical interactive and headless paths run cleanly on host
- the canonical interactive device path builds cleanly on all three supported targets
- the repo has fewer competing truths and less framework ceremony in normal product development

At that point, remaining work should be feature work or bug fixes, not more architecture planning.
