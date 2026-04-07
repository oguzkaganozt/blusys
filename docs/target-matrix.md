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
| **I/O & Communication** | `gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `twai`, `i2s`, `i2s_rx`, `rmt`, `rmt_rx`, `one_wire` | `touch` — ESP32, S3 only |
| **Analog** | `adc`, `sdm`, `pwm` | `dac` — ESP32 only |
| **Timers & Counters** | `timer` | `pcnt`, `mcpwm` — ESP32, S3 only |
| **Storage** | `nvs`, `sd_spi` | `sdmmc` — ESP32, S3 only |
| **Device** | `system`, `sleep`, `wdt` | `temp_sensor` — C3, S3 only |

### Services

| Category | Modules |
|----------|---------|
| **Display** | `lcd`, `led_strip` |
| **Input** | `button`, `encoder` |
| **Actuator** | `buzzer` |
| **Connectivity** | `wifi`, `wifi_prov`, `espnow`, `bluetooth`, `ble_gatt`, `mdns` |
| **Protocol** | `mqtt`, `http_client`, `http_server`, `ws_client` |
| **System** | `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota` |

All service modules are available on all targets.

### Target-Specific Summary

| Module | ESP32 | ESP32-C3 | ESP32-S3 |
|--------|:-----:|:--------:|:--------:|
| `pcnt` | :material-check:{ .green } | — | :material-check:{ .green } |
| `touch` | :material-check:{ .green } | — | :material-check:{ .green } |
| `dac` | :material-check:{ .green } | — | — |
| `sdmmc` | :material-check:{ .green } | — | :material-check:{ .green } |
| `temp_sensor` | — | :material-check:{ .green } | :material-check:{ .green } |
| `mcpwm` | :material-check:{ .green } | — | :material-check:{ .green } |

Unsupported modules return `BLUSYS_ERR_NOT_SUPPORTED` at runtime. Use `blusys_target_supports()` to check before calling.

## Requirements

- **ESP-IDF v5.5+** — auto-detected by `blusys` CLI
- **`bluetooth`/`ble_gatt`** — require NimBLE enabled via `CONFIG_BT_NIMBLE_ENABLED` in sdkconfig
- **Networking modules** (`http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`) — require WiFi connected first
- **`mdns`** — additionally requires `espressif/mdns` managed component in the project's `idf_component.yml`

## Intentionally Out of Scope

- Advanced I2S modes beyond standard TX/RX
- Touch sensor behavior beyond polling
- DAC modes beyond oneshot output
- LP peripheral special cases
