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

- `components/blusys/` and `components/blusys_services/` expose C headers with `extern "C"` guards
- `components/blusys_framework/` is the only C++ tier in V1; it exposes C++ headers
- framework C++ uses `-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics`
- framework C++ should stay platform-facing and disciplined, not become a second copy of ESP-IDF internals
- services migration to C++ is deferred to V2
- keep cross-tier boundaries explicit: framework depends on services, services depend on HAL + drivers

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
- during the v7 refactor, prefer the new canonical product path over preserving additive continuity with the old one

## Development Workflow

1. confirm the scope against `PRD.md` and `ROADMAP.md`
2. define whether the work belongs to the canonical product path, an advanced path, or validation-only infrastructure
3. define the public API shape and ownership boundary
4. define lifecycle and thread-safety rules
5. implement the smallest change that moves the repo toward the v7 target
6. add or update only the docs, examples, and validation required for that support tier
7. validate the affected paths proportionally

### Change Rules

- start from the smallest correct implementation
- keep the public surface smaller than the backend surface
- avoid target-specific public APIs unless there is a clear need
- keep target differences internal
- during the v7 refactor, prefer deliberate simplification over preserving broad additive surface area

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
2. run the hardware smoke tests in `guides/hardware-smoke-tests.md`
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

- v7 product requirements: `../PRD.md`
- v7 roadmap: `../ROADMAP.md`
- repository guidance for agents: `../CLAUDE.md`
