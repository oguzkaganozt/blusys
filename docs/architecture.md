# Architecture

## Layer Model

Blusys HAL uses one stable public surface and hides backend complexity behind internal layers.

```text
Application
-> blusys public API
-> blusys module implementation
-> blusys ESP-IDF port layer
-> ESP-IDF drivers and system services
-> ESP32-C3 / ESP32-S3
```

## Architectural Intent

- public headers should be application-facing
- internal code may depend on ESP-IDF details
- target-specific differences must stay behind internal boundaries
- the public API must remain usable without reading ESP-IDF docs for common cases

## Internal Layers

### Public API Layer

Contains all exported `blusys_` headers.

Rules:
- no `esp_err_t` in public headers
- no public ESP-IDF handle types
- no board-specific naming
- only stable, documented user-facing types

### Module Layer

Contains module-specific logic such as validation, state handling, error translation, and concurrency behavior.

Rules:
- prefer minimal wrappers only when they improve usability
- centralize lifecycle handling
- centralize thread-safety policy

### ESP-IDF Port Layer

Contains thin bindings to ESP-IDF 5.5 driver APIs such as `esp_driver_gpio`, `esp_driver_i2c`, `esp_driver_spi`, `esp_driver_uart`, `esp_driver_ledc`, and `esp_driver_gptimer`.

Rules:
- no direct exposure of port-layer types to the public API
- map `esp_err_t` to `blusys_err_t`
- keep backend-specific conditionals local to this layer when possible

### Target Layer

Contains capability and per-target adjustments for ESP32-C3 and ESP32-S3.

Rules:
- keep a common public API
- only the internal layer may branch on target capabilities
- add extension modules later instead of contaminating the common API

## Repository Layout

Recommended implementation layout:

```text
include/blusys/
src/common/
src/internal/
src/port/esp_idf/
src/targets/esp32c3/
src/targets/esp32s3/
examples/
tests/
docs/
```

## Public Module Categories

- foundational: `version`, `error`, `target`, `system`
- simple stateless peripheral API: `gpio`
- handle-based peripheral API: `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`

## Resource Ownership Model

Use a mixed model:

- GPIO is stateless and pin-based
- UART, I2C, SPI, PWM, ADC, and timer are handle-based

Lifecycle conventions:
- `open`
- `close`
- `start`
- `stop`
- `read`
- `write`
- `transfer`

## Target Policy

The v1 API is limited to the common subset of ESP32-C3 and ESP32-S3.

Keep these out of the common v1 API:
- S3-only feature sets
- LP peripheral special cases
- advanced I2S modes
- touch sensor
- DAC
- SDMMC and storage-specific public abstractions

## Documentation Policy

Every public module must ship with:
- one task-first guide
- one API reference page
- one buildable example
- documented target support and limitations
