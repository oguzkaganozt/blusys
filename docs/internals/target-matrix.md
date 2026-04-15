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

### Drivers

| Category | Modules |
|----------|---------|
| **Display** | `lcd`, `led_strip`, `seven_seg` |
| **Input** | `button`, `encoder` |
| **Sensor** | `dht` |
| **Actuator** | `buzzer` |

All driver modules build through `blusys` and use the same target-support rules as the HAL beneath them.

### Services

| Category | Modules |
|----------|---------|
| **Display / Runtime** | `ui` |
| **Input / Runtime** | `usb_hid` |
| **Connectivity** | `wifi`, `wifi_prov`, `espnow`, `bluetooth`, `ble_gatt`, `mdns` |
| **Protocol** | `mqtt`, `http_client`, `http_server`, `ws_client` |
| **System** | `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl` |

All service modules are available on all targets when their sdkconfig and managed-component requirements are met. For `usb_hid`, BLE transport is available on all targets; USB transport additionally requires ESP32-S3 USB host support. `ui` additionally requires the `lvgl/lvgl` managed component.

### Framework

| Category | Modules | Notes |
|----------|---------|-------|
| **Product API** | `blusys::app` (`app_spec`, `app_ctx`, `app_services`, `entry`), profiles (`host`, `headless`, `st7735`), capabilities (`connectivity`, `storage`, `bluetooth`, `ota`, `diagnostics`, `telemetry`, `lan_control`, `usb`) | All three targets |
| **View layer** | `view::` helpers, stock widgets (`bu_button`, `bu_toggle`, `bu_slider`, `bu_knob`, `bu_gauge`, `bu_chart`, …), encoder focus helpers | Gated by `BLUSYS_BUILD_UI`; host builds via SDL2 |
| **Core spine** | `router`, `intent`, `feedback`, `runtime`, containers | Framework-internal; not called by product code |

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

Unsupported modules return `BLUSYS_ERR_NOT_SUPPORTED` at runtime. Use `blusys_target_supports()` to check target-level availability before calling, then also satisfy any sdkconfig or managed-component requirements listed below.

## Requirements

- **ESP-IDF v5.5+** — auto-detected by `blusys` CLI
- **`bluetooth`** — require `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_CENTRAL=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y`, and `CONFIG_BT_NIMBLE_ROLE_OBSERVER=y`
- **`ble_gatt`** — require `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, and `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`
- **`wifi_prov`** (BLE transport) — additionally requires `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y`
- **Networking modules** (`http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`, `local_ctrl`) — require WiFi connected first
- **`mdns`** — additionally requires `espressif/mdns` managed component in the project's `idf_component.yml`
- **`usb_device`** — additionally requires `espressif/esp_tinyusb` managed component in the project's `idf_component.yml`
- **`usb_hid`** (BLE transport) — additionally requires `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_CENTRAL=y`, and `CONFIG_BT_NIMBLE_ROLE_OBSERVER=y`
- **`usb_hid`** (USB transport) — additionally requires `espressif/usb_host_hid` managed component in the project's `idf_component.yml`
- **`ui`** — additionally requires `lvgl/lvgl` managed component in the project's `idf_component.yml`
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
