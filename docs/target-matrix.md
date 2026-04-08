# Compatibility

## Supported Targets

| Target | Status |
|--------|--------|
| ESP32 | :material-check-circle:{ .green } Supported |
| ESP32-C3 | :material-check-circle:{ .green } Supported |
| ESP32-S3 | :material-check-circle:{ .green } Supported |

## Module Support

All modules below are available on **all three targets** unless marked otherwise.

### HAL

| Category | Modules | Notes |
|----------|---------|-------|
| **I/O & Communication** | `gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `twai`, `i2s`, `i2s_rx`, `rmt`, `rmt_rx`, `one_wire`, `gpio_expander` | `touch` — ESP32, S3 only; `usb_host`, `usb_device` — ESP32-S3 only |
| **Analog** | `adc`, `sdm`, `pwm` | `dac` — ESP32 only |
| **Timers & Counters** | `timer` | `pcnt`, `mcpwm` — ESP32, S3 only |
| **Storage** | `nvs`, `sd_spi` | `sdmmc` — ESP32, S3 only |
| **Device** | `system`, `sleep`, `wdt`, `efuse` | `temp_sensor` — C3, S3 only; `ulp` — ESP32, S3 only |

### Services

| Category | Modules |
|----------|---------|
| **Display** | `lcd`, `led_strip`, `seven_seg`, `ui` |
| **Input** | `button`, `encoder`, `usb_hid` |
| **Sensor** | `dht` |
| **Actuator** | `buzzer` |
| **Connectivity** | `wifi`, `wifi_prov`, `espnow`, `bluetooth`, `ble_gatt`, `mdns` |
| **Protocol** | `mqtt`, `http_client`, `http_server`, `ws_client` |
| **System** | `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl` |

All service modules are available on all targets. For `usb_hid`, BLE transport is available on all targets; USB transport additionally requires ESP32-S3 USB host support. `ui` additionally requires the `espressif/lvgl` managed component.

### Target-Specific Summary

| Module | ESP32 | ESP32-C3 | ESP32-S3 |
|--------|:-----:|:--------:|:--------:|
| `pcnt` | :material-check:{ .green } | — | :material-check:{ .green } |
| `touch` | :material-check:{ .green } | — | :material-check:{ .green } |
| `dac` | :material-check:{ .green } | — | — |
| `sdmmc` | :material-check:{ .green } | — | :material-check:{ .green } |
| `temp_sensor` | — | :material-check:{ .green } | :material-check:{ .green } |
| `mcpwm` | :material-check:{ .green } | — | :material-check:{ .green } |
| `ulp` | :material-check:{ .green } | — | :material-check:{ .green } |
| `usb_host` | — | — | :material-check:{ .green } |
| `usb_device` | — | — | :material-check:{ .green } |

Unsupported modules return `BLUSYS_ERR_NOT_SUPPORTED` at runtime. Use `blusys_target_supports()` to check before calling.

## Requirements

- **ESP-IDF v5.5+** — auto-detected by `blusys` CLI
- **`bluetooth`/`ble_gatt`** — require NimBLE enabled via `CONFIG_BT_NIMBLE_ENABLED` in sdkconfig
- **Networking modules** (`http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`, `local_ctrl`) — require WiFi connected first
- **`mdns`** — additionally requires `espressif/mdns` managed component in the project's `idf_component.yml`
- **`usb_device`** — additionally requires `espressif/esp_tinyusb` managed component in the project's `idf_component.yml`
- **`usb_hid`** (USB transport) — additionally requires `espressif/usb_host_hid` managed component in the project's `idf_component.yml`
- **`ui`** — additionally requires `espressif/lvgl` managed component in the project's `idf_component.yml`
- **`ulp`** — additionally requires `CONFIG_ULP_COPROC_ENABLED=y`, `CONFIG_ULP_COPROC_TYPE_FSM=y`, and reserved ULP RTC memory in sdkconfig

## Deferred Modules

These modules are planned but deferred because they are not supported on any current Blusys target.

| Module | Required Hardware | Supported Chips (ESP-IDF) | Status |
|--------|-------------------|---------------------------|--------|
| `ana_cmpr` | Analog comparator (`SOC_ANA_CMPR_SUPPORTED`) | ESP32-C5, ESP32-H2, ESP32-P4, ESP32-C61 | Deferred — revisit when a supported target is added |

## Intentionally Out of Scope

- Advanced I2S modes beyond standard TX/RX
- Touch sensor behavior beyond polling
- DAC modes beyond oneshot output
- LP peripheral special cases
