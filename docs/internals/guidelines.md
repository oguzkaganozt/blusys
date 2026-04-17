# Guidelines

Start by reading the [Architecture](architecture.md) page.

## API Design Rules

### Core Rules

- public C APIs in HAL, drivers, and services use the `blusys_` prefix
- product-facing C++ APIs may use the `blusys::...` namespace hierarchy
- HAL, drivers, and services expose C headers and must not expose ESP-IDF types
- names should be direct and readable
- keep the common path short and easy to call

### Language Policy

- `components/blusys/` HAL, drivers, and services layers expose C headers with `extern "C"` guards
- `components/blusys/` framework layer is the only C++ tier; it exposes C++ headers in `include/blusys/framework/`
- framework C++ uses `-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics`
- framework C++ should stay platform-facing and disciplined, not become a second copy of ESP-IDF internals
- services migration to C++ is out of scope; the services tier stays C
- keep cross-tier boundaries explicit: framework depends on services, services depend on HAL + drivers
- integration bridge: `capability_event` may carry `payload`; unmapped raw integration IDs can appear as `integration_passthrough` when `app_spec::map_event` is set (see `capability_event.hpp`)

See [Architecture](architecture.md) for the tier model.

### Logging Convention

- HAL and service code uses `esp_log.h` directly (`ESP_LOGI`, `ESP_LOGE`, etc.)
- framework code uses `blusys/log.h` (`BLUSYS_LOGI`, `BLUSYS_LOGE`, etc.) — a thin
  wrapper that isolates the framework tier from direct `esp_log.h` usage
- product app code may use either, but preferring `blusys/log.h` keeps the style
  consistent with framework code

### Allocation Policy

- framework code (core spine + widget kit) does not call `new` / `delete` or
  `malloc` / `free` after initialization
- interactive widget callback slots use fixed-capacity static pools sized by
  KConfig symbols (`BLUSYS_UI_<NAME>_POOL_SIZE`); the pool fires a fail-loud assert
  on exhaustion — silent pool exhaustion is worse than a crash
- layout primitives and the core spine (router, runtime, feedback bus) are
  allocation-free after `init()`

### API Shape

- use `blusys_err_t` for public results
- use opaque handles for stateful peripherals
- use raw GPIO numbers publicly
- prefer simple scalar arguments for common operations
- use config structs only when the simple path is no longer enough

### Error and Behavior Rules

- translate backend errors internally
- keep the public error model small
- document major failure cases
- treat blocking APIs as first-class
- add async only where it is naturally useful

### Thread-Safety Rules

- define thread-safety per module
- protect lifecycle operations such as `close` against concurrent active use
- document ISR-safe behavior explicitly

### Stability Rules

- do not expose target-specific behavior unless all supported targets share it
- do not add or recommend public API without proportionate examples, docs, and validation
- prefer the canonical product path over preserving additive continuity with legacy sketches

### Framework product API versioning

Breaking changes to `app_spec`, `app_ctx`, `app_fx`, and umbrella `blusys/app/*.hpp` headers are announced alongside the framework tier (changelog / release notes when applicable) and should be accompanied by updates to canonical docs and quickstarts when they touch the recommended product path. For the runtime story (queues, threading, what not to do in `update()`), see [App runtime model](../app/app-runtime-model.md).

### Product application layout (enforced in CI)

- **Canonical `main/` shape:** `core/` (state, actions, `update()`), `ui/` (screens, LVGL, sync — may be empty for headless with a README), `platform/` (`app_spec`, capabilities, `map_event`, entry macro in `platform/app_main.cpp`).
- **Validation examples** (`examples/validation/`) and other non-framework demos are exempt; see `scripts/check-product-layout.py`.
- Optional inventory flags: `layout_exempt`, `product_layout` (see `inventory.yml` header).

For how the **framework** composes LVGL flex, scroll, and the interaction shell (primitives, shell `content_area`, stock widget sizing), see [Architecture — UI layout](architecture.md#ui-layout-lvgl-flex).

### Product application C++ shape {#product-application-cpp-shape}

- **Reducer center:** domain rules live in `update(ctx, state, action)`; keep navigation and capability sync as explicit actions.
- **Strong ids:** use `enum class` for routes and overlays in product code; `ctx.fx().nav.to` / `ctx.fx().nav.show_overlay` accept those enums.
- **State from factories:** use `ctx.product_state<YourState>()` in screen factories instead of file-scope pointers to `app_state`.
- **View sync:** prefer `blusys::` bindings and small composites (for example `sync_percent_output`, `sync_line_chart_series`) over calling `blusys::ui::*` setters directly from product code.
- **Route handles:** group `lv_obj_t*` handles in per-route structs with `clear()` on hide; the screen registry still owns teardown of the widget tree.
- **Settings:** give interactive `setting_item` rows a non-zero `id` when using `settings_screen_config::on_changed`; the callback receives that stable id (or the row index when `id` is 0).
- **Platform wiring:** include `blusys/framework/app/` headers when you need typed capability event IDs or `dispatch_variant`; use `blusys/framework/app/build_profile.hpp` (or `reference_build_profile.hpp`) to avoid duplicated `#ifdef` matrices in `platform/app_main.cpp`.

## Development Workflow

1. confirm the scope against `README.md` (**Product foundations**) and this guide
2. define whether the work belongs to the canonical product path, an advanced path, or validation-only infrastructure
3. define the public API shape and ownership boundary
4. define lifecycle and thread-safety rules
5. implement the smallest change that advances the canonical product path
6. add or update only the docs, examples, and validation required for that support tier
7. validate the affected paths proportionally

### Change Rules

- start from the smallest correct implementation
- keep the public surface smaller than the backend surface
- avoid target-specific public APIs unless there is a clear need
- keep target differences internal
- prefer deliberate simplification over preserving broad additive surface area

### Release Rules

- no undocumented public API
- no unsupported recommended-path API
- no release cut without validation appropriate to the API's support tier

## Module Done Criteria

A public surface is done when it has support artifacts appropriate to its support tier.

Canonical product-path features should generally have:

- public API
- implementation
- at least one canonical example or quickstart path
- user-facing documentation
- successful builds on the supported targets for that path
- validation appropriate to its release role

Advanced or validation-only surfaces may require a different mix of examples, docs, and validation depending on their role in the repo.

## Testing Strategy

### Goals

- verify builds on all supported targets
- verify runtime behavior on real boards
- verify lifecycle and concurrency behavior where it matters

### Validation Layers

1. build all shipped examples for `esp32`, `esp32c3`, and `esp32s3`
2. run the hardware smoke tests in `docs/internals/testing/hardware-smoke-tests.md`
3. run the concurrency examples when changing lifecycle, locking, or callback behavior

### Testing Rules

- test the public API, not internal ESP-IDF details
- keep hardware checks small and repeatable
- add regression coverage when bugs are fixed

## Documentation Standards

### Writing Rules

- write for application developers first
- explain the task before listing APIs
- prefer short examples over long theory
- avoid repeating setup, build, and flash instructions across many pages
- keep user docs task-first, module docs reference-first

### Module Reference Page Shape

- purpose
- supported targets
- quick example
- lifecycle
- blocking and async behavior
- thread safety
- ISR notes when relevant
- limitations and error behavior

### Task Guide Shape

- problem statement
- prerequisites
- minimal example
- APIs used
- expected behavior
- common mistakes

## Project Tracking

- product foundations: `../README.md` (**Product foundations**)

## Maintaining

The single merged component and its `REQUIRES` name:

| Directory | Component | Role |
|-----------|-----------|------|
| `components/blusys/` | `blusys` | All four layers (hal/drivers/services/framework); public headers under `include/blusys/` |

Dependency direction within the component: framework → services → drivers → hal → ESP-IDF. The `blusys/` include namespace is shared across all layers.

### Suggested reading order

1. Repository root `README.md` — product foundations
2. [Architecture](architecture.md) — tiers and layering
3. This page — API and workflow
4. Repository root `inventory.yml` — modules, examples, docs (CI manifest)

### Pre-merge / pre-release checks

Aligned with `.github/workflows/ci.yml`:

- `./blusys lint` — layering (`scripts/lint-layering.sh`) + framework UI source list consistency
- `python3 scripts/check-inventory.py`
- `python3 scripts/check-framework-ui-sources.py` (after CMake or widget path changes)
- `mkdocs build --strict` (any doc or `mkdocs.yml` change)
- Representative builds: `blusys host-build` (e.g. `scripts/host` or a quickstart example with `host/`), and `blusys build` on at least one HAL validation example when components change

For broader changes, also run `python3 scripts/check-product-layout.py` and `scripts/scaffold-smoke.sh`.

## Integration baseline (metrics)

Record before/after snapshots when shrinking `main/platform/` or adding framework-owned flows.

```bash
python3 scripts/measure_integration_baseline.py
```

The script prints lines of code in `main/platform/` and counts `map_event` / `map_intent` function bodies (heuristic) for reference examples.

The default action queue capacity is `app_runtime<State, Action, 16>` unless a project overrides the third template argument. See [App runtime model](../app/app-runtime-model.md) for overflow behavior and `action_queue_drop_count()`.
