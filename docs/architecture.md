# Architecture

Blusys keeps one public API and hides ESP-IDF and target-specific details behind internal layers.

## Layer Model

```text
Application
-> blusys public API
-> module implementation
-> ESP-IDF port layer
-> ESP-IDF drivers
-> ESP32 / ESP32-C3 / ESP32-S3
```

## Core Rules

- public headers are application-facing
- internal code may depend on ESP-IDF details
- target-specific behavior stays behind internal boundaries
- the public API should stay usable without reading ESP-IDF docs for common tasks

## Internal Structure

- public API layer: exported `blusys_` headers only
- module layer: validation, lifecycle, concurrency, and error translation
- port layer: thin ESP-IDF bindings and `esp_err_t` translation
- target layer: capability and per-target adjustments

## Public API Shape

- foundational modules: `version`, `error`, `target`, `system`
- stateless pin API: `gpio`
- handle-based master/TX APIs: `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`, `rmt`, `i2s`, `twai`, `pcnt`, `dac`, `sdm`, `mcpwm`, `touch`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`
- handle-based slave/RX counterparts: `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`

Symmetric pairs expose the same peripheral in both directions under separate handles:

- I2C: `blusys_i2c_master_*` and `blusys_i2c_slave_*`
- SPI: `blusys_spi_*` (master) and `blusys_spi_slave_*`
- I2S: `blusys_i2s_tx_*` and `blusys_i2s_rx_*`
- RMT: `blusys_rmt_*` (TX) and `blusys_rmt_rx_*`

Lifecycle verbs stay explicit where relevant:

- `open`
- `close`
- `start`
- `stop`
- `read`
- `write`
- `transfer`

## Target Policy

The v1 API is limited to the common subset of `esp32`, `esp32c3`, and `esp32s3`.

Keep these out of the common API:

- advanced I2S modes
- touch sensor behavior beyond the shipped polling subset
- DAC modes beyond the shipped oneshot subset
- LP peripheral special cases
- storage-specific abstractions

## Documentation Contract

Every public module should ship with:

- one example
- one task guide
- one reference page
- documented target support and limitations
