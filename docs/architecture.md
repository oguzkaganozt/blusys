# Architecture

Blusys is organized as two ESP-IDF components that share the `blusys/` header namespace. The HAL layer wraps hardware drivers; the Services layer builds on the HAL to provide higher-level application building blocks.

## Guiding Principles

- keep the public API small and readable
- support one shared API across `esp32`, `esp32c3`, and `esp32s3`
- hide target-specific and ESP-IDF-specific details internally
- keep docs task-first and reference pages short

**Success means:** users can complete common tasks from Blusys docs first, examples build on all supported targets, and normal application code does not need target-specific `#ifdef` logic.

**Non-goals:** board support helpers inside the core HAL, wrapping every ESP-IDF feature, exposing internal HAL or LL details publicly.

## Two-Component Structure

```text
components/blusys/            — HAL (hardware abstraction)
components/blusys_services/   — Services (protocols, devices, system services)
```

Services depend on HAL (`REQUIRES blusys` in CMakeLists.txt). HAL never depends on Services.

## Layer Model

```text
Application
  → Services API            (components/blusys_services/include/blusys/<category>/*.h)
  → Service Implementation  (components/blusys_services/src/<category>/*.c)
  → HAL Public API          (components/blusys/include/blusys/*.h)
  → HAL Implementation      (components/blusys/src/common/*.c)
  → Internal Utilities      (components/blusys/include/blusys/internal/)
  → Target Capabilities     (components/blusys/src/targets/esp32*/target_caps.c)
  → ESP-IDF Drivers
  → ESP32 / ESP32-C3 / ESP32-S3
```

## Core Rules

- public headers are application-facing
- internal code may depend on ESP-IDF details
- target-specific behavior stays behind internal boundaries
- HAL modules never depend on Services modules
- internal utilities live in `include/blusys/internal/` and are shared by both components

## HAL Modules (`components/blusys/`)

- foundational: `version`, `error`, `target`, `system`
- stateless pin API: `gpio`
- handle-based master/TX APIs: `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`, `rmt`, `i2s`, `twai`, `pcnt`, `dac`, `sdm`, `mcpwm`, `touch`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `usb_host`, `usb_device`
- handle-based slave/RX counterparts: `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`
- storage: `nvs`, `sd_spi`
- external bus-backed peripherals: `one_wire`, `gpio_expander`

Umbrella header: `#include "blusys/blusys.h"`

## Services Modules (`components/blusys_services/`)

Services are organized into 7 categories, each with its own subdirectory under `include/blusys/` and `src/`:

- display: `lcd`, `led_strip`, `seven_seg`, `ui`
- input: `button`, `encoder`, `usb_hid`
- sensor: `dht`
- actuator: `buzzer`
- connectivity: `wifi`, `wifi_prov`, `espnow`, `bluetooth`, `ble_gatt`, `mdns`
- protocol: `mqtt`, `http_client`, `http_server`, `ws_client`
- system: `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`

Umbrella header: `#include "blusys/blusys_services.h"`

Combined umbrella: `#include "blusys/blusys_all.h"`

## Symmetric Pairs

Symmetric pairs expose the same peripheral in both directions under separate handles:

- I2C: `blusys_i2c_master_*` and `blusys_i2c_slave_*`
- SPI: `blusys_spi_*` (master) and `blusys_spi_slave_*`
- I2S: `blusys_i2s_tx_*` and `blusys_i2s_rx_*`
- RMT: `blusys_rmt_*` (TX) and `blusys_rmt_rx_*`

## CMakeLists.txt Usage

HAL-only examples:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys)
```

Examples that use any service module:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys_services)
```

Both components are auto-discovered via `EXTRA_COMPONENT_DIRS ../../components` in the project-level CMakeLists.txt.

## Lifecycle Verbs

Lifecycle verbs stay explicit where relevant:

- `open`
- `close`
- `start`
- `stop`
- `read`
- `write`
- `transfer`

## Target Policy

The API targets the common subset of `esp32`, `esp32c3`, and `esp32s3`. Modules not available on all three targets use SOC capability guards and return `BLUSYS_ERR_NOT_SUPPORTED` on unsupported hardware. See [Compatibility](target-matrix.md) for the full matrix.
