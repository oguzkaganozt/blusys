# Architecture

Blusys is in transition from a HAL-first library repo to an internal product platform. The repo now has a three-tier component shape:

```text
components/blusys/            HAL + drivers
components/blusys_services/   runtime services
components/blusys_framework/  framework infrastructure (transition in progress)
```

The public include namespace stays `blusys/...` across all tiers.

## Guiding Principles

- keep the public surface smaller than raw ESP-IDF
- support one shared platform across `esp32`, `esp32c3`, and `esp32s3`
- keep target-specific and ESP-IDF-specific details behind internal boundaries
- give product code a clear answer to "where does this belong?"

**Success means:** examples build on all supported targets, the layering rules are obvious, and product code does not need to understand low-level ESP-IDF details just to get started.

**Non-goals:** mirroring every ESP-IDF feature, exposing internal HAL details publicly, or adding board-specific product helpers to the core platform.

## Three-Tier Model

```text
Product / example app
  → Framework API           (components/blusys_framework/include/blusys/framework/...)
  → Services API            (components/blusys_services/include/blusys/<category>/*.h)
  → HAL + drivers API       (components/blusys/include/blusys/*.h,
                              components/blusys/include/blusys/drivers/<category>/*.h)
  → HAL + drivers impl      (components/blusys/src/common/*.c,
                              components/blusys/src/drivers/<category>/*.c)
  → Internal helpers        (components/blusys/include/blusys/internal/)
  → Target capability data  (components/blusys/src/targets/esp32*/target_caps.c)
  → ESP-IDF
```

Dependency direction is one-way:

```text
blusys_framework → blusys_services → blusys
```

Reverse dependencies are forbidden.

## Tier Details

### HAL + Drivers (`components/blusys/`)

This component now contains two internal layers.

**HAL modules**
- foundational: `version`, `error`, `target`, `system`, `efuse`
- retained low-power task engine: `ulp`
- stateless pin API: `gpio`
- handle-based master/TX APIs: `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`, `rmt`, `i2s`, `twai`, `pcnt`, `dac`, `sdm`, `mcpwm`, `touch`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `usb_host`, `usb_device`
- handle-based slave/RX counterparts: `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`
- storage: `nvs`, `sd_spi`
- external bus-backed peripherals: `one_wire`, `gpio_expander`

**Driver modules**
- display: `lcd`, `led_strip`, `seven_seg`
- input: `button`, `encoder`
- sensor: `dht`
- actuator: `buzzer`

Directory split:
- HAL implementation: `components/blusys/src/common/`
- driver implementation: `components/blusys/src/drivers/<category>/`
- HAL public headers: `components/blusys/include/blusys/*.h`
- driver public headers: `components/blusys/include/blusys/drivers/<category>/*.h`

Umbrella header:

```c
#include "blusys/blusys.h"
```

### Services (`components/blusys_services/`)

Services are stateful runtime modules built on HAL + drivers.

- display/runtime: `ui`
- input/runtime: `usb_hid`
- connectivity: `wifi`, `wifi_prov`, `wifi_mesh`, `espnow`, `bluetooth`, `ble_gatt`, `mdns`
- protocol: `mqtt`, `http_client`, `http_server`, `ws_client`
- system: `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl`

`ui` stays here because it owns LVGL lifecycle, render-task behavior, and display flush orchestration. `usb_hid` stays here because it spans USB-host and BLE runtime behavior rather than acting as a simple hardware driver.

Umbrella header:

```c
#include "blusys/blusys_services.h"
```

### Framework (`components/blusys_framework/`)

The framework tier has been introduced as a real ESP-IDF component, but only minimal infrastructure is shipped during this stage of the transition.

Intended long-term role:
- `core/` for product lifecycle, controllers, intents, routing, and feedback
- `ui/` for higher-level UI helpers and widget-kit code

Current status:
- component exists in the build graph
- only foundational framework headers are shipped so far
- framework pages in the docs are placeholders until implementation lands

## Layering Rules

- public headers are application-facing
- internal code may depend on ESP-IDF details
- target-specific behavior stays behind internal boundaries
- services never depend on framework
- HAL/common code must not include driver headers

The HAL/drivers boundary is enforced by `blusys lint`, backed by `scripts/lint-layering.sh`.

The lint checks exactly two rules:
- no file under `components/blusys/src/common/` may include `blusys/drivers/**`
- files under `components/blusys/src/drivers/` may only include the shared internal allowlist: `blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h`

## Symmetric Pairs

Some HAL modules expose the same peripheral in both directions under separate handles:

- I2C: `blusys_i2c_master_*` and `blusys_i2c_slave_*`
- SPI: `blusys_spi_*` and `blusys_spi_slave_*`
- I2S: `blusys_i2s_tx_*` and `blusys_i2s_rx_*`
- RMT: `blusys_rmt_*` and `blusys_rmt_rx_*`

## CMake Usage

HAL-only or driver examples:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys)
```

Service examples:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys_services)
```

Framework examples and future product apps:

```cmake
idf_component_register(SRCS "main.cpp" REQUIRES blusys_framework)
```

The repo's bundled examples are discovered via `EXTRA_COMPONENT_DIRS ../../components` in each example project's top-level `CMakeLists.txt`.

Prefer per-tier umbrella headers. `blusys/blusys_all.h` still exists as a transitional compatibility header, but new code should include the specific tier headers instead.

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

The platform targets the common subset of `esp32`, `esp32c3`, and `esp32s3`. Modules not available on all three targets use capability checks and return `BLUSYS_ERR_NOT_SUPPORTED` on unsupported hardware. See [Compatibility](target-matrix.md) for the full matrix.
