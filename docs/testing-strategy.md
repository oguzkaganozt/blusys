# Testing Strategy

## Goals

- verify builds on all supported targets
- verify runtime behavior on real boards
- verify lifecycle and concurrency behavior where it matters

## Validation Layers

1. build all shipped examples for `esp32`, `esp32c3`, and `esp32s3`
2. run the hardware smoke tests in `guides/hardware-smoke-tests.md`
3. run the concurrency examples when changing lifecycle, locking, or callback behavior

## Module Done Criteria

A public module is done when it has:

- public API
- implementation
- example
- task guide
- module reference page
- successful builds on all supported targets
- at least one real-board validation pass

## Testing Rules

- test the public API, not internal ESP-IDF details
- keep hardware checks small and repeatable
- add regression coverage when bugs are fixed
