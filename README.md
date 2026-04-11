# Blusys Platform

An internal ESP32 product platform built on ESP-IDF v5.5. Three tiers
sharing the `blusys/` header namespace, one supported set of targets
(**ESP32**, **ESP32-C3**, **ESP32-S3**), and a single CLI for the whole
build/flash/host workflow.

## Quick Start

```sh
# Install (once)
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
~/.blusys/install.sh

# Scaffold a product (interactive controller archetype by default)
mkdir ~/my_product && cd ~/my_product
blusys create --archetype interactive-controller
# or: blusys create --archetype edge-node   # headless-first connected archetype

# Build, flash, monitor
blusys run /dev/ttyACM0 esp32s3
```

Requires ESP-IDF v5.5+ (auto-detected). `blusys create` generates a
minimal app using the `blusys::app` reducer model. The canonical interactive
reference is now `examples/quickstart/interactive_controller/`, with
`examples/reference/interactive_panel/` as the secondary interactive archetype.
See
[Getting Started](docs/start/index.md) for the full walkthrough.

## Architecture

Three ESP-IDF components, one-way dependency direction:

```
Application
  → Framework      blusys/framework/...        (C++; controllers, routing,
                                                 feedback, widget kit)
  → Services       blusys/<category>/<module>  (C; runtime modules)
  → HAL + Drivers  blusys/<module> /            (C; hardware abstraction +
                   blusys/drivers/<...>          driver building blocks)
  → ESP-IDF
  → Hardware
```

`blusys_framework` → `blusys_services` → `blusys`. Reverse dependencies
are forbidden and the HAL/drivers boundary inside `components/blusys/`
is enforced by `blusys lint`.

### HAL modules

| Category | Modules |
|---|---|
| **I/O & Communication** | `gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `twai`, `i2s`, `i2s_rx`, `rmt`, `rmt_rx`, `one_wire`, `gpio_expander`, `touch`, `usb_host`, `usb_device` |
| **Analog** | `adc`, `dac`, `sdm`, `pwm` |
| **Timers & Counters** | `timer`, `pcnt`, `mcpwm` |
| **Storage** | `nvs`, `sdmmc`, `sd_spi` |
| **Device** | `system`, `sleep`, `wdt`, `temp_sensor`, `efuse`, `ulp` |

### Driver modules

| Category | Modules |
|---|---|
| **Display** | `lcd`, `led_strip`, `seven_seg` |
| **Input** | `button`, `encoder` |
| **Sensor** | `dht` |
| **Actuator** | `buzzer` |

### Service modules

| Category | Modules |
|---|---|
| **Display / Runtime** | `ui` |
| **Input / Runtime** | `usb_hid` |
| **Connectivity** | `wifi`, `wifi_prov`, `wifi_mesh`, `espnow`, `bluetooth`, `ble_gatt`, `mdns` |
| **Protocol** | `mqtt`, `http_client`, `http_server`, `ws_client` |
| **System** | `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl` |

### Framework + App Layer

C++ tier shipping the `blusys::app` product API and the internal
framework spine:

- **App model:** `app_spec`, `app_ctx`, reducer-driven dispatch
  (`update(ctx, state, action)`), entry macros (`BLUSYS_APP_MAIN_HOST`,
  `_HEADLESS`, `_DEVICE`).
- **View layer:** action-bound widgets, reactive bindings, page helpers,
  custom widget contract, bounded LVGL scope.
- **Platform profiles:** host (SDL2), headless, generic SPI ST7735.
- **Capabilities:** connectivity (Wi-Fi, SNTP, mDNS, local control),
  storage (SPIFFS, FAT), diagnostics, provisioning, OTA, bluetooth.
- **Widget kit:** `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`,
  `bu_overlay`, plus layout primitives `screen` / `row` / `col` /
  `label` / `divider`.
- **Theme system:** single `theme_tokens` struct populated at boot.

See [`components/blusys_framework/widget-author-guide.md`](components/blusys_framework/widget-author-guide.md)
for the six-rule widget contract.

## Usage

Product code (recommended):

```cpp
#include "blusys/app/app.hpp"          /* app model, entry macros, view layer */
```

HAL and services (direct access):

```c
#include "blusys/blusys.h"             /* HAL + drivers */
#include "blusys/blusys_services.h"    /* services */
```

```cmake
# HAL + driver examples
idf_component_register(SRCS "main.c" REQUIRES blusys)

# Service examples
idf_component_register(SRCS "main.c" REQUIRES blusys_services)

# Framework / app examples (C++)
idf_component_register(SRCS "main.cpp" REQUIRES blusys_framework)
```

## Host harness

The `scripts/host/` CMake project builds LVGL against SDL2 on Linux so
the widget kit can be iterated against without flashing hardware on
every change. See [`scripts/host/README.md`](scripts/host/README.md) for
the install steps.

```sh
sudo apt install libsdl2-dev   # or distro equivalent
blusys host-build
./scripts/host/build-host/hello_lvgl
```

## Examples

Examples are organized into three categories:

- `examples/quickstart/` --- archetype starters (`blusys create` shapes) using `blusys::app`
- `examples/reference/` --- deeper demos, connectivity examples, and framework validation builds
- `examples/validation/` --- internal stress tests and smoke tests

See `inventory.yml` for the full classification and CI inclusion flags.

## Documentation

[**oguzkaganozt.github.io/blusys**](https://oguzkaganozt.github.io/blusys/) —
organized as Start, App, Services, HAL + Drivers, Internals, Archive.

Build locally:

```sh
pip install -r requirements-docs.txt
mkdocs serve
```

## Project Status

Current release: **v7.0.0**. See [`ROADMAP.md`](ROADMAP.md) for current state
and release history. See [`CLAUDE.md`](CLAUDE.md) for repo conventions.

## License

See [`LICENSE`](LICENSE).
