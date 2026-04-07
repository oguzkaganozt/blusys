# Progress

This is the single source of truth for both roadmap and implementation progress.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- current phase: `V5`
- overall status: `in_progress`
- last completed milestone: `V4`
- next target milestone: `V5`
- current release: `v5.0.0`
- open blockers: none

## Roadmap

### Completed

- foundation and project setup
- core modules: `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`
- async support and validation work
- `v1.0.0` release
- full `V2`: `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`
- `v2.0.0` release
- full `V3`: `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt`
- `v3.0.0` release
- full `V4`: `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`
- standalone `blusys` CLI with install-once workflow, IDF auto-detection, tab completion
- `v4.0.0` release
- V5 early work: `encoder`, `buzzer`, `one_wire`, services restructure into 7 categories, docs HAL/Services split
- `v5.0.0` release
- LCD: add ST7735R driver support with GRAM offset configuration
- LCD: add SSD1306 128x32 I2C OLED example and docs

### V2

Core HAL expansion.

- status: `completed`
- done: `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`

### V3

Connectivity and system services.

- status: `completed`
- done: `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt`

### V4

Production essentials and standalone CLI.

- status: `completed`
- done: `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`
- tooling: standalone `blusys` CLI, `install.sh`, IDF auto-detection, `config-idf`, `example` command, bash tab completion

### V5

Advanced peripherals, sensor drivers, and architecture improvements.

- status: `in_progress`
- done: `encoder`, `buzzer`, `one_wire`, services restructure (7 categories), docs consolidation (HAL/Services nav split, 8→3 project pages)
- planned:
  1. first sensor driver (TBD)
  2. `gpio_expander` — I2C/SPI-based port expanders
  3. `ana_cmpr` — analog signal comparison (C3, S3 only)
  4. `efuse` — one-time programmable memory reading
  5. `ethernet` — wired networking (SPI W5500, or native EMAC on ESP32)
  6. `ulp` — ultra low power coprocessor programming (ESP32, S3 only)
  7. `wifi_mesh` — lightweight mesh networking via `esp_mesh_lite`
  8. `usb_hid` — USB HID (S3 USB-OTG) and BLE HID (all targets)
  9. `camera` — camera interface via `esp32-camera` (ESP32, S3 only)
  10. `local_ctrl` — local device control over BLE/WiFi

## Milestones

| Milestone | Status | Notes |
|---|---|---|
| Foundation | completed | project setup, component skeleton, target plumbing, smoke app |
| Core Modules | completed | `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer` |
| Async And Validation | completed | timer callbacks, GPIO interrupt callbacks, UART async, hardware validation |
| V1 Release | completed | `v1.0.0` |
| V2 | completed | `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx` — released `v2.0.0` |
| V3 | completed | `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt` — released `v3.0.0` |
| V4 | completed | `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`, standalone CLI — released `v4.0.0` |
| V5 | in_progress | `encoder`, `buzzer`, `one_wire`, services restructure, docs overhaul — released `v5.0.0` |

## Recent Work

- restructured docs nav into HAL/Services split, consolidated project pages from 8 to 3
- restructured `blusys_services` into 7 categories: display, input, sensor, actuator, connectivity, protocol, system
- added `one_wire` HAL module: Dallas/Maxim 1-Wire protocol
- added `buzzer` service module: PWM-driven passive piezo, single-tone and sequence playback
- added `encoder` service module: EC11-style rotary encoders with PCNT hardware acceleration

## Current Technical State

### HAL Public API (`components/blusys/`)

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
- `blusys_nvs_*`
- `blusys_sd_spi_*`
- `blusys_one_wire_*`

### Services Public API (`components/blusys_services/`)

**display:** `blusys_lcd_*`, `blusys_led_strip_*`
**input:** `blusys_button_*`, `blusys_encoder_*`
**actuator:** `blusys_buzzer_*`
**connectivity:** `blusys_wifi_*`, `blusys_wifi_prov_*`, `blusys_espnow_*`, `blusys_bluetooth_*`, `blusys_ble_gatt_*`, `blusys_mdns_*`
**protocol:** `blusys_mqtt_*`, `blusys_http_client_*`, `blusys_http_server_*`, `blusys_ws_client_*`
**system:** `blusys_fs_*`, `blusys_fatfs_*`, `blusys_console_*`, `blusys_pm_*`, `blusys_sntp_*`, `blusys_ota_*`

### Internal Infrastructure

- two-component architecture: HAL (`blusys`) and Services (`blusys_services`)
- services organized into 7 categories with subdirectory layout
- target-specific source selection in component CMake
- target capability reporting for shipped public modules
- shared lock abstraction for handle-based modules (`include/blusys/internal/`)
- shared timeout and ESP-IDF error translation helpers

## Validation Snapshot

- `v1.0.0` release validation completed
- `v2.0.0` release validation completed
- `v3.0.0` release validation completed
- `v4.0.0` release validation completed
- `v5.0.0` release validation completed
- `mkdocs build --strict` passes

## Environment Notes

- ESP-IDF v5.5+ required; auto-detected by `blusys` CLI
- install: `git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys && ~/.blusys/install.sh`
- some examples use target-specific `sdkconfig` files

## Next Actions

1. Implement first sensor service driver
2. Continue V5 planned modules
