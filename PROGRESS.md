# Project Progress

This file tracks implementation progress against the roadmap in `docs/roadmap.md`.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- Current phase: `Phase 1: Foundation`
- Overall status: `in_progress`
- Last completed milestone: project documentation baseline and Phase 1 scaffold
- Next target milestone: `Phase 2: Foundational Public Modules`

## Phase Tracking

| Phase | Status | Notes |
|---|---|---|
| Phase 0: Project Freeze | completed | architecture, rules, roadmap, docs baseline created |
| Phase 1: Foundation | completed | component skeleton, foundation headers/sources, target capability plumbing, smoke app, dual-target build validation |
| Phase 2: Foundational Public Modules | not_started | `system` and `gpio` planned next |
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

## Current Technical State

Public API currently exists for:
- `blusys_version_*`
- `blusys_err_string()`
- `blusys_target_*`

Internal infrastructure currently exists for:
- target-specific source selection in the component CMake
- target capability reporting
- shared lock abstraction for future handle-based modules

## Open Items Before Phase 2

- define exact `system` API surface
- define exact `gpio` API surface
- decide whether `system` should include uptime in Phase 2 or wait until later
- extend docs with module-specific pages once `system` and `gpio` exist

## Known Environment Notes

- ESP-IDF 5.5.4 local environment required missing `tree_sitter` packages before builds worked
- dual-target validation uses separate `sdkconfig` files per target:
  - `examples/smoke/sdkconfig.esp32c3`
  - `examples/smoke/sdkconfig.esp32s3`

## Next Actions

1. implement `system`
2. implement `gpio`
3. extend smoke validation or add first dedicated examples
4. document `system` and `gpio`
