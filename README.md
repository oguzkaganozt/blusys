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

# Scaffold a product (interactive: framework UI tier on; headless: UI tier off)
mkdir ~/my_product && cd ~/my_product
blusys create --starter interactive   # or --starter headless

# Build, flash, monitor
blusys run /dev/ttyACM0 esp32s3
```

Requires ESP-IDF v5.5+ (auto-detected). `blusys create` generates the
four-CMakeLists product layout (top-level + `main/` + `app/` +
`app/product_config.cmake`); platform components are pulled by ESP-IDF's
managed component manager from `main/idf_component.yml`. See
[Getting Started](docs/guides/getting-started.md) for the full walkthrough.

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

### Framework

C++ tier shipping the V1 widget kit and the product spine:

- **Core spine:** `router`, `intent`, `feedback`, `controller`, `runtime`
  (queued events, route delivery, feedback bus, tick cadence).
- **Widget kit:** `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`,
  `bu_overlay`, plus layout primitives `screen` / `row` / `col` /
  `label` / `divider`.
- **Theme system:** single `theme_tokens` struct populated at boot.
- **Encoder helpers:** `create_encoder_group` + `auto_focus_screen` for
  encoder-driven focus traversal across the widget kit.

See [`components/blusys_framework/widget-author-guide.md`](components/blusys_framework/widget-author-guide.md)
for the six-rule widget contract.

## Usage

```c
#include "blusys/blusys.h"             /* HAL + drivers */
#include "blusys/blusys_services.h"    /* services */
```

```cpp
#include "blusys/framework/framework.hpp"  /* framework core */
#include "blusys/framework/ui/widgets.hpp" /* widget kit */
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

## Documentation

[**oguzkaganozt.github.io/blusys**](https://oguzkaganozt.github.io/blusys/) —
guides, API reference, and architecture docs.

Build locally:

```sh
pip install -r requirements-docs.txt
mkdocs serve
```

## Project Status

Current release: **v6.1.0**. See [`ROADMAP.md`](ROADMAP.md) for current state,
release history, and planned work. See [`CLAUDE.md`](CLAUDE.md) for repo conventions.

## License

See [`LICENSE`](LICENSE).
