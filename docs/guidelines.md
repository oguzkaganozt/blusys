# Guidelines

Start by reading the [Architecture](architecture.md) page.

## API Design Rules

### Core Rules

- all public symbols use the `blusys_` prefix
- public API is C only and must not expose ESP-IDF types
- names should be direct and readable
- keep the common path short and easy to call

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
- repository guidance for agents: `../AGENTS.md`
