# Project Progress

This file tracks implementation progress against `docs/roadmap.md`.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- current phase: `V2`
- overall status: `in_progress`
- last completed milestone: `V2: pcnt`
- next target milestone: `V2: rmt`
- open blockers: none

## Milestones

| Milestone | Status | Notes |
|---|---|---|
| Foundation | completed | project setup, component skeleton, target plumbing, smoke app |
| Core Modules | completed | `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer` |
| Async And Validation | completed | timer callbacks, GPIO interrupt callbacks, UART async, hardware validation |
| Release | completed | `v1.0.0` |
| V2 | in_progress | `pcnt` done, `rmt` next |
| V3 | not_started | `usb`, `wifi`, `bluetooth`, `eth`, `nvs`, `ota` |
| V4 | not_started | `efuse`, `ulp`, advanced power, BSP, diagnostics, security, service helpers |

## Recent Work

- released `v1.0.0`
- simplified docs and project guidance
- aligned docs and scripts around supported targets: `esp32`, `esp32c3`, `esp32s3`
- added `pcnt` API, implementation, example, and docs
- added watch-point callback support for `pcnt`
- gated `pcnt` by target support in the current ESP-IDF baseline:
  - supported: `esp32`, `esp32s3`
  - not supported: `esp32c3`

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
- `blusys_pcnt_*`

Internal infrastructure currently exists for:

- target-specific source selection in component CMake
- target capability reporting for shipped public modules
- shared lock abstraction for handle-based modules
- shared timeout and ESP-IDF error translation helpers

## Validation Snapshot

- `v1.0.0` release validation completed
- `pcnt_basic` builds pass for:
  - `esp32`
  - `esp32c3`
  - `esp32s3`
- `smoke` builds pass for:
  - `esp32c3`
  - `esp32s3`
- `mkdocs build --strict` passes

## Environment Notes

- ESP-IDF version in use: `5.5.4`
- if `export.sh` looks for a missing ESP-IDF Python env, set `IDF_PYTHON_ENV_PATH` to the installed env under `~/.espressif/python_env/`
- some examples use target-specific `sdkconfig` files

## Next Actions

1. continue `V2` with `rmt`
2. keep `pcnt` limited to watch points unless a concrete encoder or multi-channel use case appears
3. continue the `V2` roadmap in order unless priorities change
