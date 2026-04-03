# Project Progress

This file tracks implementation progress against the roadmap in `docs/roadmap.md`.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- Current phase: `Phase 5: Async And Hardening`
- Overall status: `in_progress`
- Last completed milestone: `Phase 4: Timing And Analog Modules`
- Next target milestone: `Phase 5: Async And Hardening`

## Phase Tracking

| Phase | Status | Notes |
|---|---|---|
| Phase 0: Project Freeze | completed | architecture, rules, roadmap, docs baseline created |
| Phase 1: Foundation | completed | component skeleton, foundation headers/sources, target capability plumbing, smoke app, dual-target build validation |
| Phase 2: Foundational Public Modules | completed | `system` and `gpio` implemented with examples, guides, and dual-target build validation |
| Phase 3: Core Serial And Bus Modules | completed | `uart`, `i2c`, and `spi` implemented with examples, guides, and dual-target build validation |
| Phase 4: Timing And Analog Modules | completed | `pwm`, `adc`, and `timer` implemented with examples, guides, and dual-target build validation |
| Phase 5: Async And Hardening | in_progress | `timer` callbacks, GPIO interrupt callbacks, and UART async implemented; build hardening is done and runtime validation remains |
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

## Open Items In Phase 5

- run focused hardware smoke validation for async lifecycle behavior
- complete the final Phase 5 review and fix pass

## Known Environment Notes

- ESP-IDF 5.5.4 local environment required missing `tree_sitter` packages before builds worked
- this machine currently needs `IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.10_env` before sourcing ESP-IDF export
- dual-target validation uses separate `sdkconfig` files per target:
  - `examples/smoke/sdkconfig.esp32c3`
  - `examples/smoke/sdkconfig.esp32s3`

## Next Actions

1. run focused hardware smoke validation for async lifecycle behavior
2. complete the Phase 5 review and fix pass
3. decide whether Phase 5 exit criteria are met
