# Compatibility

## Supported Targets

Blusys supports these targets:

- `esp32`
- `esp32c3`
- `esp32s3`

## Module Support Matrix

All modules listed under **All Targets** are available on ESP32, ESP32-C3, and ESP32-S3. Modules in the **Target-Specific** table are only available on the indicated targets.

### All Targets

These modules are in the base feature mask and work on every supported target:

| Category | Modules |
|----------|---------|
| Core Peripherals | `gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave` |
| Analog | `adc`, `sdm`, `pwm` |
| Timers & Counters | `timer`, `rmt`, `rmt_rx` |
| Bus | `twai`, `i2s`, `i2s_rx` |
| System | `system`, `target`, `version`, `error`, `nvs`, `fs`, `wdt`, `sleep` |
| Networking | `wifi`, `http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `bluetooth`, `ble_gatt`, `espnow` |

### Target-Specific Modules

| Module | ESP32 | ESP32-C3 | ESP32-S3 |
|--------|:-----:|:--------:|:--------:|
| `pcnt` | yes | — | yes |
| `touch` | yes | — | yes |
| `dac` | yes | — | — |
| `sdmmc` | yes | — | yes |
| `temp_sensor` | — | yes | yes |
| `mcpwm` | yes | — | yes |

## Compatibility Rules

- The public API is the same across supported targets
- Application code should not need target-specific `#ifdef` logic for normal use
- Target-specific implementation details stay internal to the library
- Unsupported modules return `BLUSYS_ERR_NOT_SUPPORTED` at runtime; use `blusys_target_supports()` to check before calling

## Practical Notes

- Some examples use board-dependent pin defaults, so check `menuconfig` when needed
- Runtime behavior can still differ because boards expose different safe pins and wiring constraints
- Thread-safety rules are defined in task terms, not CPU-core terms
- `bluetooth` and `ble_gatt` require NimBLE enabled via `CONFIG_BT_NIMBLE_ENABLED` in sdkconfig
- Networking modules (`http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`) require WiFi to be connected first
- `mdns` additionally requires the `espressif/mdns` managed component in the project's `idf_component.yml`

## Intentionally Limited Scope

These are intentionally not part of the current public surface:

- Advanced I2S feature sets beyond standard TX/RX
- Touch sensor support beyond polling
- DAC support beyond oneshot output
- LP peripheral variants
