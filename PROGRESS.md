# Project Progress

This file tracks implementation progress against the roadmap in `docs/roadmap.md`.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- Current phase: `Phase 7: Release`
- Overall status: `completed`
- Last completed milestone: `Phase 7: Release`
- Next target milestone: `post-v1 maintenance`

## Phase Tracking

| Phase | Status | Notes |
|---|---|---|
| Phase 0: Project Freeze | completed | architecture, rules, roadmap, docs baseline created |
| Phase 1: Foundation | completed | component skeleton, foundation headers/sources, target capability plumbing, smoke app, multi-target build validation |
| Phase 2: Foundational Public Modules | completed | `system` and `gpio` implemented with examples, guides, and multi-target build validation |
| Phase 3: Core Serial And Bus Modules | completed | `uart`, `i2c`, and `spi` implemented with examples, guides, and multi-target build validation |
| Phase 4: Timing And Analog Modules | completed | `pwm`, `adc`, and `timer` implemented with examples, guides, and multi-target build validation |
| Phase 5: Async And Hardening | completed | `timer` callbacks, GPIO interrupt callbacks, and UART async implemented and hardware-validated |
| Phase 6: Stabilization | completed | full example build matrix passed; docs cleanup and release review completed |
| Phase 7: Release | completed | `v1.0.0` prepared as the first stable public release |

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
- `docs/modules/v1-core.md` (removed later during docs simplification)

### Phase 1 Foundation

- created `components/blusys/` ESP-IDF component structure
- added public foundation headers for `version`, `error`, and `target`
- added foundation source files
- added internal lock abstraction over FreeRTOS mutexes
- added target capability plumbing for `esp32`, `esp32c3`, and `esp32s3`
- added `examples/smoke/` validation app
- validated smoke app builds for:
  - `esp32c3`
  - `esp32s3`

### Phase 2 Foundational Public Modules

- added public `system` header and implementation
- added public `gpio` header and implementation
- added `examples/system_info/`
- added `examples/gpio_basic/`
- added module docs for `system` and `gpio`
- validated examples build for:
  - `esp32c3`
  - `esp32s3`

### Phase 3 Core Serial And Bus Modules

- added public `uart` header and implementation
- added public `i2c` master header and implementation
- added public `spi` master header and implementation
- added `examples/uart_loopback/`
- added `examples/i2c_scan/`
- added `examples/spi_loopback/`
- added module docs for `uart`, `i2c`, and `spi`
- aligned `blusys_target_supports()` feature reporting with currently shipped public module support
- validated examples build for:
  - `esp32c3`
  - `esp32s3`

### Phase 4 Timing And Analog Modules

- added public `pwm` header and implementation
- added public `adc` header and implementation
- added public `timer` header and implementation
- added `examples/pwm_basic/`
- added `examples/adc_basic/`
- added `examples/timer_basic/`
- added module docs for `pwm`, `adc`, and `timer`
- validated example builds for:
  - `esp32c3`
  - `esp32s3`

### Phase 5 Async And Hardening

- added direct ISR timer callback registration to the `timer` module
- added direct ISR GPIO callback registration to the `gpio` module
- added task-context UART async TX and RX callback registration to the `uart` module
- completed focused hardware validation for `gpio_interrupt`, `uart_async`, and `timer_basic`
- completed hardware validation for communication examples including `uart_loopback`, `i2c_scan`, and `spi_loopback`

### Phase 6 Stabilization

- validated the full shipped example build matrix with `build_examples.sh`
- aligned project docs and agent guidance to the supported target set: `esp32`, `esp32c3`, and `esp32s3`

### Phase 7 Release

- set public version metadata to `1.0.0`
- added `CHANGELOG.md` for the first stable release
- finalized docs and status files for the `v1.0.0` release

## Current Technical State

Public API currently exists for:
- `blusys_version_*`
- `blusys_err_string()`
- `blusys_target_*`
- `blusys_system_*`
- `blusys_gpio_*`
- `blusys_uart_*`
- `blusys_i2c_master_*`
- `blusys_spi_*`
- `blusys_pwm_*`
- `blusys_adc_*`
- `blusys_timer_*`

Internal infrastructure currently exists for:
- target-specific source selection in the component CMake
- target capability reporting for implemented public modules
- shared lock abstraction for handle-based modules
- shared internal timeout and ESP-IDF error translation helpers

## Open Items

- no blocking items remain for `v1.0.0`

## Known Environment Notes

- ESP-IDF 5.5.4 local environment required missing `tree_sitter` packages before builds worked
- if `export.sh` looks for a missing ESP-IDF Python env, set `IDF_PYTHON_ENV_PATH` to the installed env directory under `~/.espressif/python_env/`
- some examples use separate `sdkconfig` files per target:
  - `examples/smoke/sdkconfig.esp32c3`
  - `examples/smoke/sdkconfig.esp32s3`

## Next Actions

1. create the release commit and `v1.0.0` tag
2. publish release notes from `CHANGELOG.md`
3. continue post-v1 maintenance and future module planning
