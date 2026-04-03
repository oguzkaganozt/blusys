# Project Progress

This file tracks implementation progress against the roadmap in `docs/roadmap.md`.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- Current phase: `Phase 3: Core Serial And Bus Modules`
- Overall status: `in_progress`
- Last completed milestone: `Phase 2: Foundational Public Modules`
- Next target milestone: `Phase 3: Core Serial And Bus Modules`

## Phase Tracking

| Phase | Status | Notes |
|---|---|---|
| Phase 0: Project Freeze | completed | architecture, rules, roadmap, docs baseline created |
| Phase 1: Foundation | completed | component skeleton, foundation headers/sources, target capability plumbing, smoke app, dual-target build validation |
| Phase 2: Foundational Public Modules | completed | `system` and `gpio` implemented with examples, guides, and dual-target build validation |
| Phase 3: Core Serial And Bus Modules | not_started | `uart`, `i2c`, `spi` |
| Phase 4: Timing And Analog Modules | not_started | `pwm`, `adc`, `timer` |
| Phase 5: Async And Hardening | not_started | GPIO interrupt callbacks, UART async, timer callbacks |
| Phase 6: Stabilization | not_started | smoke tests, concurrency tests, docs cleanup |
| Phase 7: Release | not_started | release candidate and `v1.0.0` |

## Completed Work

### Project Setup

- initialized git repository
- pushed initial repository state to `origin/main`
- added docs site configuration with `MkDocs Material`
- added docs dependency file

### Documentation Baseline

- `docs/vision.md`
- `docs/architecture.md`
- `docs/api-design-rules.md`
- `docs/target-matrix.md`
- `docs/roadmap.md`
- `docs/testing-strategy.md`
- `docs/development-workflow.md`
- `docs/documentation-standards.md`
- `docs/modules/v1-core.md`

### Phase 1 Foundation

- created `components/blusys/` ESP-IDF component structure
- added public foundation headers for `version`, `error`, and `target`
- added foundation source files
- added internal lock abstraction over FreeRTOS mutexes
- added target capability plumbing for `esp32c3` and `esp32s3`
- added `examples/smoke/` validation app
- validated smoke app builds for:
  - `esp32c3`
  - `esp32s3`

### Phase 2 Foundational Public Modules

- added public `system` header and implementation
- added public `gpio` header and implementation
- added `examples/system_info/`
- added `examples/gpio_basic/`
- added module docs and task-first guides for `system` and `gpio`
- validated examples build for:
  - `esp32c3`
  - `esp32s3`

## Current Technical State

Public API currently exists for:
- `blusys_version_*`
- `blusys_err_string()`
- `blusys_target_*`
- `blusys_system_*`
- `blusys_gpio_*`

Internal infrastructure currently exists for:
- target-specific source selection in the component CMake
- target capability reporting
- shared lock abstraction for future handle-based modules

## Open Items Before Phase 3

- define the exact blocking `uart` API surface
- define the exact blocking `i2c` master API surface
- define the exact blocking `spi` master API surface
- define thread-safety and lifecycle rules for handle-based communication modules

## Known Environment Notes

- ESP-IDF 5.5.4 local environment required missing `tree_sitter` packages before builds worked
- this machine currently needs `IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.10_env` before sourcing ESP-IDF export
- dual-target validation uses separate `sdkconfig` files per target:
  - `examples/smoke/sdkconfig.esp32c3`
  - `examples/smoke/sdkconfig.esp32s3`

## Next Actions

1. implement `uart`
2. implement `i2c` master
3. implement `spi` master
4. add examples and docs for each Phase 3 module
