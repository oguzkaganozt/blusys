# Guidelines

Start by reading the [Architecture](architecture.md) page.

## API Design Rules

### Core Rules

- all public symbols use the `blusys_` prefix
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
- do not add public API before examples and docs exist
- prefer additive evolution over breaking redesign

## Development Workflow

1. confirm the scope
2. define the public API shape
3. define lifecycle and thread-safety rules
4. implement the module
5. add the example
6. add the task guide and module reference
7. validate all supported targets

### Change Rules

- start from the smallest correct implementation
- keep the public surface smaller than the backend surface
- avoid target-specific public APIs unless there is a clear need
- keep target differences internal
- prefer small additive changes over broad redesigns

### Release Rules

- no undocumented public API
- no exampleless public module
- no module considered done without validation on all supported targets

## Module Done Criteria

A public module is done when it has:

- public API
- implementation
- example
- task guide
- module reference page
- successful builds on all supported targets
- at least one real-board validation pass

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

- implementation progress: `../PROGRESS.md`
- repository guidance for agents: `../CLAUDE.md`
