# Changelog

## Unreleased

Architecture restructuring.

- split single `components/blusys/` into two components: `components/blusys/` (HAL) and `components/blusys_services/` (Services)
- HAL layer: 29 hardware abstraction modules (gpio, uart, spi, i2c, adc, timers, etc.)
- Services layer: 19 higher-level modules (wifi, mqtt, http, bluetooth, lcd, led_strip, etc.)
- promoted internal utilities from `src/internal/` to `include/blusys/internal/` for cross-component sharing
- new umbrella headers: `blusys/blusys_services.h` (services only), `blusys/blusys_all.h` (both layers)
- `blusys/blusys.h` now includes only HAL modules
- service examples use `REQUIRES blusys_services` in CMakeLists.txt; HAL-only examples remain `REQUIRES blusys`
- no public API changes — user `#include` paths are unchanged

## v4.0.0

Production essentials and standalone CLI release.

Included in `v4.0.0`:

- all modules from `v3.0.0`
- external devices: `button` (GPIO-based debounced button), `led_strip` (addressable LED strip via RMT), `lcd` (SPI/I2C LCD panel driver)
- storage: `fatfs` (FAT filesystem on SD/SPI flash), `sd_spi` (SD card over SPI bus)
- system services: `console` (interactive UART console with command registration), `power_mgmt` (dynamic frequency scaling and light sleep)
- connectivity: `ws_client` (WebSocket client), `wifi_prov` (WiFi provisioning via BLE or SoftAP)
- standalone `blusys` CLI: install once via `install.sh`, create projects anywhere, IDF auto-detection
- `config-idf` for interactive ESP-IDF version selection
- bash tab completion for all commands, targets, ports, and example names
- all 9 new modules available on all three targets
- buildable examples for all new modules
- full API reference and task guide documentation

Validation summary:

- all V4 examples build for `esp32`, `esp32c3`, `esp32s3`
- `mkdocs build --strict` passes

## v3.0.0

Connectivity and system services release.

Included in `v3.0.0`:

- all modules from `v2.0.0`
- connectivity: `wifi` (station-mode connect/disconnect), `http_client` (blocking GET/POST), `http_server` (embedded HTTP server with URI handlers), `mqtt` (publish/subscribe client), `ota` (over-the-air firmware update), `sntp` (NTP time synchronization), `mdns` (zero-config service advertisement and discovery), `espnow` (connectionless peer-to-peer wireless)
- bluetooth: `bluetooth` (BLE advertising and scanning), `ble_gatt` (BLE GATT server with services and characteristics)
- storage: `nvs` (key-value non-volatile storage), `fs` (SPIFFS filesystem for internal flash)
- internal infrastructure: shared NVS initialization helper, widened feature mask from `uint32_t` to `uint64_t`, singleton pattern for `bluetooth`, `ble_gatt`, `espnow`, and `sntp`
- all 12 new modules available on all three targets: `esp32`, `esp32c3`, `esp32s3`
- buildable examples for all new modules
- full API reference and task guide documentation for all new modules
- `mkdocs build --strict` passes

Validation summary:

- all V3 examples build for `esp32`, `esp32c3`, `esp32s3`
- `wifi_basic`, `http_client_basic`, `bluetooth_basic` hardware smoke tests passed
- `fs_basic` hardware smoke test passed (ESP32)
- remaining V3 examples pending hardware smoke tests

## v2.0.0

Core HAL expansion release.

Included in `v2.0.0`:

- all modules from `v1.0.0`
- peripheral expansion: `pcnt`, `rmt` (TX and RX), `twai`, `i2s` (TX and RX), `touch`, `dac`, `sdmmc`, `temp_sensor`
- system services: `wdt`, `sleep`, `mcpwm`, `sdm`
- symmetric counterparts: `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`
- target-gated modules: `pcnt` (esp32, esp32s3), `touch` (esp32, esp32s3), `dac` (esp32 only), `sdmmc` (esp32, esp32s3), `temp_sensor` (esp32c3, esp32s3), `mcpwm` (esp32, esp32s3)
- all other modules available on all three targets
- buildable examples for all new modules
- full API reference and task guide documentation for all 22 modules

Validation summary:

- all V2 examples build for `esp32`, `esp32c3`, `esp32s3`
- hardware validation completed for shipped examples

## v1.0.0

First stable public release of Blusys HAL.

Included in `v1.0.0`:

- supported targets: `esp32`, `esp32c3`, `esp32s3`
- foundational modules: `version`, `error`, `target`, `system`
- digital IO: `gpio`, including interrupt callbacks
- communication: `uart`, `i2c`, `spi`, including UART async TX/RX callbacks
- timing and analog: `pwm`, `adc`, `timer`, including timer callbacks
- buildable examples for shipped modules
- simplified user documentation and module reference pages

Validation summary:

- full shipped example build matrix passed
- async hardware validation completed for `gpio_interrupt`, `uart_async`, and `timer_basic`
- communication example validation completed for `uart_loopback`, `i2c_scan`, and `spi_loopback`

Known scope limits for `v1.0.0`:

- the API is intentionally limited to the common surface across supported targets
- advanced chip-specific features remain out of scope for the core HAL
- formal unit-test infrastructure is still deferred; validation is build- and hardware-based
