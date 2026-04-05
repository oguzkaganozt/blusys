# Progress

This is the single source of truth for both roadmap and implementation progress.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- current phase: `V3`
- overall status: `in_progress`
- last completed milestone: `V2`
- next target milestone: `V3: wifi or nvs`
- open blockers: none

## Roadmap

### Completed

- foundation and project setup
- core modules: `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`
- async support and validation work
- `v1.0.0` release
- first `V2` items: `pcnt`, `rmt`
- full `V2`: `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`
- symmetric counterparts: `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`
- `v2.0.0` release

### V2

Core HAL expansion.

- status: `completed`
- done: `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`

### V3

Connectivity and system services.

- status: `not_started`
- planned: `usb`, `wifi`, `bluetooth`, `eth`, `nvs`, `ota`

### V4

Advanced and ecosystem-level helpers.

- status: `not_started`
- planned: `efuse`, `ulp`, advanced power, BSP, diagnostics, security, provisioning, higher-level service helpers

## Milestones

| Milestone | Status | Notes |
|---|---|---|
| Foundation | completed | project setup, component skeleton, target plumbing, smoke app |
| Core Modules | completed | `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer` |
| Async And Validation | completed | timer callbacks, GPIO interrupt callbacks, UART async, hardware validation |
| Release | completed | `v1.0.0` |
| V2 | completed | `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx` — released `v2.0.0` |
| V3 | not_started | `usb`, `wifi`, `bluetooth`, `eth`, `nvs`, `ota` |
| V4 | not_started | `efuse`, `ulp`, advanced power, BSP, diagnostics, security, service helpers |

## Recent Work

- released `v1.0.0`
- simplified docs and project guidance
- aligned docs and scripts around supported targets: `esp32`, `esp32c3`, `esp32s3`
- added `pcnt` API, implementation, example, and docs
- added watch-point callback support for `pcnt`
- added `rmt` TX API, implementation, example, and docs
- added `twai` classic TX API, RX callback support, example, and docs
- added `i2s` standard-mode master TX API, implementation, example, and docs
- added `touch` polling API, implementation, example, and docs
- added `dac` oneshot API, implementation, example, and docs
- gated `pcnt` by target support in the current ESP-IDF baseline:
  - supported: `esp32`, `esp32s3`
  - not supported: `esp32c3`
- gated `dac` by target support in the current ESP-IDF baseline:
  - supported: `esp32`
  - not supported: `esp32c3`, `esp32s3`
- added `sdmmc` SD card read/write API, implementation, example, and docs
- added `temp_sensor` on-chip temperature sensor API, implementation, example, and docs
- added `wdt` task watchdog API, implementation, and example
- added `sleep` light/deep sleep API, implementation, and example
- added `mcpwm` complementary pair output API, implementation, and example
- completed `V2` milestone
- added `sdm` sigma-delta modulation API, implementation, example, and docs
- `sdm` available on all three targets: `esp32`, `esp32c3`, `esp32s3`
- added `i2c_slave` slave-mode I2C API, implementation, and example
- added `spi_slave` slave-mode SPI API, implementation, and example
- added `i2s_rx` I2S receive API, implementation, and example
- added `rmt_rx` RMT receive API, implementation, and example
- all four symmetric counterparts available on all three targets
- completed full documentation coverage: all 22 modules have API reference and task guide
- released `v2.0.0`

## Current Technical State

Public API currently exists for:

- `blusys_version_*`
- `blusys_err_string()`
- `blusys_target_*`
- `blusys_system_*`
- `blusys_gpio_*`
- `blusys_uart_*`
- `blusys_i2c_master_*`
- `blusys_i2c_slave_*`
- `blusys_i2s_tx_*`
- `blusys_i2s_rx_*`
- `blusys_spi_*`
- `blusys_spi_slave_*`
- `blusys_pwm_*`
- `blusys_adc_*`
- `blusys_timer_*`
- `blusys_pcnt_*`
- `blusys_rmt_*`
- `blusys_rmt_rx_*`
- `blusys_twai_*`
- `blusys_touch_*`
- `blusys_dac_*`
- `blusys_sdmmc_*`
- `blusys_temp_sensor_*`
- `blusys_wdt_*`
- `blusys_sleep_*`
- `blusys_mcpwm_*`
- `blusys_sdm_*`

Internal infrastructure currently exists for:

- target-specific source selection in component CMake
- target capability reporting for shipped public modules
- shared lock abstraction for handle-based modules
- shared timeout and ESP-IDF error translation helpers

## Validation Snapshot

- `v1.0.0` release validation completed
- `v2.0.0` release validation completed
- all V2 examples build for `esp32`, `esp32c3`, `esp32s3`:
  - `pcnt_basic`, `rmt_basic`, `rmt_rx_basic`
  - `twai_basic`
  - `i2s_basic`, `i2s_rx_basic`
  - `touch_basic`, `dac_basic`
  - `sdmmc_basic`, `temp_sensor_basic`
  - `wdt_basic`, `sleep_basic`
  - `mcpwm_basic`, `sdm_basic`
  - `i2c_slave_basic`, `spi_slave_basic`
- `smoke` builds pass for:
  - `esp32c3`
  - `esp32s3`
- `mkdocs build --strict` passes

## Environment Notes

- ESP-IDF version in use: `5.5.4`
- if `export.sh` looks for a missing ESP-IDF Python env, set `IDF_PYTHON_ENV_PATH` to the installed env under `~/.espressif/python_env/`
- some examples use target-specific `sdkconfig` files

## Next Actions

1. begin `V3` — pick first item from: `usb`, `wifi`, `bluetooth`, `eth`, `nvs`, `ota`
2. keep `pcnt` limited to watch points unless a concrete encoder or multi-channel use case appears
3. keep the first `twai` cut limited to classic frames, blocking TX, and RX callbacks until a concrete need for filters, recovery, or CAN FD appears
4. keep the first `touch` cut limited to one-pin polling reads until a concrete need appears for thresholds, callbacks, or sleep integration
5. keep the first `dac` cut limited to oneshot output until a concrete need appears for cosine generation or continuous DMA output
6. keep the first `i2s` cut limited to standard-mode master TX/RX until a concrete need appears for duplex or alternate formats
