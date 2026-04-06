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
- full `V3`: `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt`
- `v3.0.0` release
- full `V4`: `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`
- standalone `blusys` CLI with install-once workflow, IDF auto-detection, tab completion
- `v4.0.0` release

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

Advanced connectivity and peripherals.

- status: `not_started`
- implementation order:
  1. `gpio_expander` — I2C/SPI-based port expanders
  2. `ana_cmpr` — analog signal comparison (C3, S3 only)
  3. `efuse` — one-time programmable memory reading
  4. `ethernet` — wired networking (SPI W5500, or native EMAC on ESP32)
  5. `ulp` — ultra low power coprocessor programming (ESP32, S3 only)
  6. `wifi_mesh` — lightweight mesh networking via `esp_mesh_lite`
  7. `usb_hid` — USB HID (S3 USB-OTG) and BLE HID (all targets)
  8. `camera` — camera interface via `esp32-camera` (ESP32, S3 only)
  9. `local_ctrl` — local device control over BLE/WiFi

## Milestones

| Milestone | Status | Notes |
|---|---|---|
| Foundation | completed | project setup, component skeleton, target plumbing, smoke app |
| Core Modules | completed | `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer` |
| Async And Validation | completed | timer callbacks, GPIO interrupt callbacks, UART async, hardware validation |
| Release | completed | `v1.0.0` |
| V2 | completed | `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx` — released `v2.0.0` |
| V3 | completed | `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt` — released `v3.0.0` |
| V4 | completed | `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`, standalone CLI — released `v4.0.0` |
| V5 | not_started | `gpio_expander`, `ana_cmpr`, `efuse`, `ethernet`, `ulp`, `wifi_mesh`, `usb_hid`, `camera`, `local_ctrl` |

## Recent Work

- released `v4.0.0`
- V4 modules: `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`
- standalone `blusys` CLI — install once, create projects anywhere, IDF auto-detection
- `install.sh` for zero-config setup via `~/.local/bin` symlink
- `config-idf` for interactive ESP-IDF version selection
- `example` command for running bundled examples from anywhere
- bash tab completion for all commands, targets, ports, and example names

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
- `blusys_wifi_*`
- `blusys_nvs_*`
- `blusys_http_client_*`
- `blusys_mqtt_*`
- `blusys_http_server_*`
- `blusys_ota_*`
- `blusys_sntp_*`
- `blusys_mdns_*`
- `blusys_bluetooth_*`
- `blusys_fs_*`
- `blusys_espnow_*`
- `blusys_ble_gatt_*`
- `blusys_button_*`
- `blusys_led_strip_*`
- `blusys_console_*`
- `blusys_fatfs_*`
- `blusys_sd_spi_*`
- `blusys_pm_*`
- `blusys_ws_client_*`
- `blusys_wifi_prov_*`
- `blusys_lcd_*`

Internal infrastructure currently exists for:

- target-specific source selection in component CMake
- target capability reporting for shipped public modules
- shared lock abstraction for handle-based modules
- shared timeout and ESP-IDF error translation helpers

## Validation Snapshot

- `v1.0.0` release validation completed
- `v2.0.0` release validation completed
- `v3.0.0` release validation completed
- `v4.0.0` release validation completed
- `mkdocs build --strict` passes

## Environment Notes

- ESP-IDF v5.5+ required; auto-detected by `blusys` CLI
- install: `git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys && ~/.blusys/install.sh`
- some examples use target-specific `sdkconfig` files

## Next Actions

1. Begin `V5` — advanced connectivity and peripherals
