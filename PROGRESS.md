# Progress

This is the single source of truth for both roadmap and implementation progress.

## Status Legend

- `not_started`
- `in_progress`
- `completed`
- `blocked`

## Current Summary

- current phase: `V4`
- overall status: `in_progress`
- last completed milestone: `V3`
- next target milestone: `V4`
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

### V2

Core HAL expansion.

- status: `completed`
- done: `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`

### V3

Connectivity and system services.

- status: `completed`
- done: `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt`

### V4

Production essentials.

- status: `in_progress`
- implementation order:
  1. `button` — GPIO-based debounce/long-press abstraction ✓
  2. `led_strip` — addressable LEDs (WS2812, SK6812) via RMT ✓
  3. `console` — interactive UART console with command registration ✓
  4. `fatfs` — FAT filesystem on internal flash with wear levelling ✓
  5. `sd_spi` — SD card over SPI bus (builds on `fatfs`) ✓
  6. `power_mgmt` — CPU frequency scaling, auto light sleep
  7. `websocket` — WebSocket client for real-time bidirectional comms
  8. `wifi_prov` — BLE/SoftAP-based WiFi credential provisioning
  9. `lcd` — SPI/I2C/parallel display drivers

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
| V4 | not_started | `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `websocket`, `wifi_prov`, `lcd` |
| V5 | not_started | `gpio_expander`, `ana_cmpr`, `efuse`, `ethernet`, `ulp`, `wifi_mesh`, `usb_hid`, `camera`, `local_ctrl` |

## Recent Work

- released `v3.0.0`
- restructured feature roadmap into V4/V5 release plan
- added `button` module (V4) — GPIO debounce and long-press abstraction
- added `led_strip` module (V4) — WS2812B addressable LED driver via RMT
- added `console` module (V4) — interactive UART REPL with command registration
- added `fatfs` module (V4) — FAT filesystem on internal flash with wear-levelling
- added `sd_spi` module (V4) — SD card over SPI with FAT filesystem

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

Internal infrastructure currently exists for:

- target-specific source selection in component CMake
- target capability reporting for shipped public modules
- shared lock abstraction for handle-based modules
- shared timeout and ESP-IDF error translation helpers

## Validation Snapshot

- `v1.0.0` release validation completed
- `v2.0.0` release validation completed
- `v3.0.0` release validation completed
- `mkdocs build --strict` passes

## Environment Notes

- ESP-IDF version in use: `5.5.4`
- if `export.sh` looks for a missing ESP-IDF Python env, set `IDF_PYTHON_ENV_PATH` to the installed env under `~/.espressif/python_env/`
- some examples use target-specific `sdkconfig` files

## Next Actions

1. continue `V4` — next module: `power_mgmt`
2. follow implementation order: `power_mgmt` → `websocket` → `wifi_prov` → `lcd`
