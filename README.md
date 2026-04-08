# Blusys HAL

A simplified C hardware abstraction layer for ESP32 devices, built on ESP-IDF v5.5.

One API across **ESP32**, **ESP32-C3**, and **ESP32-S3** — simpler than raw ESP-IDF for common embedded tasks.

## Quick Start

```sh
# Install (once)
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
~/.blusys/install.sh

# Create, build, flash, monitor
mkdir ~/my_project && cd ~/my_project
blusys create
blusys run /dev/ttyACM0 esp32s3
```

Requires ESP-IDF v5.5+ (auto-detected).

## Architecture

Two ESP-IDF components sharing the `blusys/` header namespace:

```
Application
  → Services    blusys/<category>/<module>.h    (high-level building blocks)
  → HAL         blusys/<module>.h               (hardware abstraction)
  → ESP-IDF
  → Hardware
```

### HAL Modules

| Category | Modules |
|---|---|
| **I/O & Communication** | `gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `twai`, `i2s`, `i2s_rx`, `rmt`, `rmt_rx`, `one_wire`, `gpio_expander`, `touch`, `usb_host`, `usb_device` |
| **Analog** | `adc`, `dac`, `sdm`, `pwm` |
| **Timers & Counters** | `timer`, `pcnt`, `mcpwm` |
| **Storage** | `nvs`, `sdmmc`, `sd_spi` |
| **Device** | `system`, `sleep`, `wdt`, `temp_sensor` |

### Services Modules

| Category | Modules |
|---|---|
| **Display** | `lcd`, `led_strip`, `seven_seg`, `ui` |
| **Input** | `button`, `encoder`, `usb_hid` |
| **Sensor** | `dht` |
| **Actuator** | `buzzer` |
| **Connectivity** | `wifi`, `wifi_prov`, `espnow`, `bluetooth`, `ble_gatt`, `mdns` |
| **Protocol** | `mqtt`, `http_client`, `http_server`, `ws_client` |
| **System** | `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota` |

## Usage

```c
#include "blusys/blusys.h"           /* HAL only */
#include "blusys/blusys_services.h"  /* all services */
#include "blusys/blusys_all.h"       /* both */
```

```cmake
# HAL only
idf_component_register(SRCS "main.c" REQUIRES blusys)

# With services
idf_component_register(SRCS "main.c" REQUIRES blusys_services)
```

## Documentation

[**oguzkaganozt.github.io/blusys**](https://oguzkaganozt.github.io/blusys/) — guides, API reference, and architecture docs.

Build locally:

```sh
pip install -r requirements-docs.txt
mkdocs serve
```

## Project Status

**v5.0.0** — 51 modules across HAL and Services, standalone `blusys` CLI, full docs site. See [`PROGRESS.md`](PROGRESS.md) for detailed tracking.

## License

See [`LICENSE`](LICENSE).
