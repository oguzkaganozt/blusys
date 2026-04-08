# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Blusys HAL** is a simplified C Hardware Abstraction Layer for ESP32 devices, built on top of ESP-IDF v5.5.4. It exposes a smaller, more stable API surface than raw ESP-IDF. Supported targets: **ESP32**, **ESP32-C3**, and **ESP32-S3**. Current release: `v5.0.0`. 62 modules across HAL and Services.

## Build and Flash Commands

All commands go through the `blusys` CLI. Default target is `esp32s3`. All commands default to the current directory when no project is specified.

```bash
blusys create [path]                                              # scaffold project (default: cwd)
blusys build          [project] [esp32|esp32c3|esp32s3]           # build only
blusys flash          [project] [port] [esp32|esp32c3|esp32s3]
blusys monitor        [project] [port] [esp32|esp32c3|esp32s3]
blusys run            [project] [port] [esp32|esp32c3|esp32s3]    # build+flash+monitor
blusys config         [project] [esp32|esp32c3|esp32s3]           # opens menuconfig
blusys menuconfig     [project] [esp32|esp32c3|esp32s3]           # alias for config
blusys size           [project] [esp32|esp32c3|esp32s3]           # firmware size breakdown
blusys erase          [project] [port] [esp32|esp32c3|esp32s3]    # erase entire flash
blusys clean          [project] [esp32|esp32c3|esp32s3]           # remove one target build dir
blusys fullclean      [project]                                   # remove all target build dirs
blusys build-examples                                             # build all examples x all targets
blusys version                                                    # show version and IDF info
blusys update                                                     # self-update via git pull
```

`blusys` configuration:
- IDF detection: auto-detects from `$IDF_PATH`, PATH, or `~/.espressif/*/esp-idf/`
- `BLUSYS_PATH` is exported automatically and used by project CMakeLists.txt

Build artifacts land in `examples/<name>/build-esp32/`, `build-esp32c3/`, `build-esp32s3/`. If Kconfig values change, delete the build directory before rebuilding — stale `sdkconfig` will silently miss new `CONFIG_*` symbols.

There is no automated test runner. Validation is via hardware smoke tests: `docs/guides/hardware-smoke-tests.md`.

## Documentation

```bash
pip install -r requirements-docs.txt
mkdocs serve          # local preview
mkdocs build --strict # must pass before any merge
```

`mkdocs build --strict` is the doc gate — it fails on broken nav references or missing pages.

The site uses MkDocs Material with navigation tabs: **Home**, **Guides**, **API Reference**, **Project**. The nav hierarchy in `mkdocs.yml` groups pages under peripheral categories (Core Peripherals, Analog, Timers & Counters, Bus, Sensors, System, Networking). When adding pages, place them in the correct category in both the Guides and API Reference tabs. Card grid landing pages live at `docs/guides/index.md` and `docs/modules/index.md` — update them when adding a new module.

## Code Architecture

### Two-Component Architecture

The codebase is split into two ESP-IDF components:

- **`components/blusys/`** — HAL layer. Thin wrappers over ESP-IDF hardware drivers (GPIO, UART, SPI, I2C, ADC, timers, etc.). No dependency on the services layer.
- **`components/blusys_services/`** — Services layer. Organized into 7 categories: display (LCD, LED strip, seven_seg, ui), input (button, encoder, usb_hid), sensor (dht), actuator (buzzer), connectivity (WiFi, Bluetooth, BLE GATT, ESP-NOW, mDNS), protocol (MQTT, HTTP, WebSocket), and system (filesystem, FATFS, console, power management, SNTP, OTA).

Services depend on HAL (`REQUIRES blusys` in CMakeLists.txt). HAL never depends on services.

```
Application
  → Services API          (components/blusys_services/include/blusys/<category>/*.h)
  → Service Impl          (components/blusys_services/src/<category>/*.c)
  → HAL Public API        (components/blusys/include/blusys/*.h)
  → HAL Implementation    (components/blusys/src/common/*.c)
  → Internal Utilities    (components/blusys/include/blusys/internal/)
  → Target Capabilities   (components/blusys/src/targets/esp32*/target_caps.c)
  → ESP-IDF Drivers
  → Hardware
```

**HAL modules:** gpio, gpio_expander, uart, i2c, i2c_slave, spi, spi_slave, adc, dac, pwm, mcpwm, timer, pcnt, rmt, rmt_rx, i2s, i2s_rx, sdm, twai, wdt, sleep, temp_sensor, touch, sdmmc, sd_spi, nvs, one_wire, efuse, ulp, usb_host, usb_device, system, error, target, version.

**Service modules** (by category):
- **display:** lcd, led_strip, seven_seg, ui
- **input:** button, encoder, usb_hid
- **sensor:** dht
- **actuator:** buzzer
- **connectivity:** wifi, wifi_prov, wifi_mesh, espnow, bluetooth, ble_gatt, mdns
- **protocol:** mqtt, http_client, http_server, ws_client
- **system:** fs, fatfs, console, power_mgmt, sntp, ota, local_ctrl

**Umbrella headers:**
- `blusys/blusys.h` — includes all HAL modules
- `blusys/blusys_services.h` — includes all service modules
- `blusys/blusys_all.h` — includes both

### Key API Conventions

- All return values use `blusys_err_t` — never `esp_err_t` in public headers.
- Stateful peripherals use an opaque handle: `blusys_<module>_open()` → `blusys_<module>_close()`.
- GPIO is stateless and pin-based (no handle).
- Timeout parameter: `BLUSYS_TIMEOUT_FOREVER` (`-1`) to block indefinitely.
- No ESP-IDF types, handles, or macros in public headers.
- No `#ifdef` for target differences in public or common code — use `blusys_target_supports(feature)` or the SOC guard pattern (see below).

### Internal Utilities (`include/blusys/internal/`)

These headers are public (usable by both `blusys` and `blusys_services`) but are implementation details, not user-facing API. Include them as `#include "blusys/internal/blusys_lock.h"`.

- `blusys_lock.h` / `blusys_lock_freertos.c` (`src/internal/`) — FreeRTOS mutex used by all stateful modules.
- `blusys_esp_err.h` — `blusys_translate_esp_err(esp_err_t)` maps ESP errors to `blusys_err_t`.
- `blusys_timeout.h` — timeout conversion helpers.
- `blusys_target_caps.h` — `BLUSYS_FEATURE_MASK(feature)` macro and `BLUSYS_BASE_FEATURE_MASK`; per-target feature bitmasks.
- `blusys_nvs_init.h` — `blusys_nvs_ensure_init()` shared helper used by any module that requires NVS (wifi, espnow, bluetooth, ble_gatt). Use this instead of calling `nvs_flash_init()` directly.

**Critical thread-safety rule:** Never hold a `blusys_lock_t` across a blocking wait (`xEventGroupWaitBits`, `vTaskDelay`, etc.). The pattern is: take lock → prepare → give lock → block. Violating this deadlocks any concurrent caller on the same handle. See `wifi.c:blusys_wifi_connect()` and `mqtt.c:blusys_mqtt_connect()` as correct reference implementations.

### Target Feature Support Matrix

Modules in `BLUSYS_BASE_FEATURE_MASK` are available on all three targets. Target-specific extras:

| Module       | ESP32 | ESP32-C3 | ESP32-S3 |
|--------------|-------|----------|----------|
| `pcnt`       | ✓     |          | ✓        |
| `touch`      | ✓     |          | ✓        |
| `dac`        | ✓     |          |          |
| `sdmmc`      | ✓     |          | ✓        |
| `temp_sensor`|       | ✓        | ✓        |
| `mcpwm`      | ✓     |          | ✓        |
| `usb_host`   |       |          | ✓        |
| `usb_device` |       |          | ✓        |

**Deferred modules (not supported on any current target):**

| Module       | Reason |
|--------------|--------|
| `ana_cmpr`   | `SOC_ANA_CMPR_SUPPORTED` is false for ESP32, ESP32-C3, and ESP32-S3; only available on ESP32-C5, ESP32-H2, ESP32-P4, ESP32-C61 |

All other modules (gpio, gpio_expander, uart, i2c, spi, pwm, adc, timer, rmt, twai, i2s, wdt, sleep, sdm, i2c_slave, spi_slave, i2s_rx, rmt_rx, one_wire, wifi, nvs, http_client, mqtt, http_server, ota, sntp, mdns, bluetooth, fs, espnow, ble_gatt, button, led_strip, console, fatfs, sd_spi, power_mgmt, ws_client, wifi_prov, lcd, encoder, buzzer, seven_seg, dht, usb_hid) are in `BLUSYS_BASE_FEATURE_MASK` and available on all targets.

**Combined headers:** `i2s.h` declares both `blusys_i2s_tx_*` and `blusys_i2s_rx_*`. `rmt.h` declares both `blusys_rmt_*` (TX) and `blusys_rmt_rx_*`. Their implementations live in separate `.c` files (`i2s.c` / `i2s_rx.c`, `rmt.c` / `rmt_rx.c`).

**Singleton modules:** `bluetooth`, `ble_gatt`, `espnow`, and `sntp` each maintain a global static handle and only support one open instance at a time. Opening a second instance returns `BLUSYS_ERR_INVALID_STATE`. They use an `ensure_global_lock()` helper and a module-level `s_*_handle` static for callback dispatch.

### Target-Gated Modules

Modules not available on all targets use a SOC capability guard (see `temp_sensor.c` or `dac.c` as reference):

```c
#include "soc/soc_caps.h"

#if SOC_X_SUPPORTED
// real implementation
#else
// stub functions returning BLUSYS_ERR_NOT_SUPPORTED
#endif
```

V3 connectivity modules (`http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`) use `SOC_WIFI_SUPPORTED` as their guard. `bluetooth` and `ble_gatt` use `CONFIG_BT_NIMBLE_ENABLED` (set via menuconfig) as their guard — they require NimBLE to be enabled in the project's sdkconfig. — they are not gated by a per-target `FEATURE_MASK` entry but by Wi-Fi hardware availability. They also require `blusys_wifi_connect()` to be called first at the application level. The `mdns` module additionally depends on the `espressif/mdns` managed component — projects that use mDNS must declare this dependency in their own `main/idf_component.yml`. See `examples/mdns_basic/main/idf_component.yml` as reference. The `blusys_services` CMakeLists.txt detects the component at build time and defines `BLUSYS_HAS_MDNS` when present.

### Examples

Each example is a standalone ESP-IDF project:
- `main/<name>_main.c` — entry point
- `main/CMakeLists.txt` — `idf_component_register(SRCS "..." REQUIRES blusys)` (or `REQUIRES blusys_services` for service modules)
- `main/Kconfig.projbuild` — menuconfig options (must be in `main/`, not project root)
- `CMakeLists.txt` — sets `EXTRA_COMPONENT_DIRS` to `../../components`
- `sdkconfig.esp32c3` / `sdkconfig.esp32s3` — optional per-target defaults

## Adding a New Module

First decide which component the module belongs to:
- **HAL** (`components/blusys/`) — direct hardware abstraction (GPIO, bus protocols, timers, ADC, etc.)
- **Services** (`components/blusys_services/`) — higher-level modules in one of 7 categories: display, input, sensor, actuator, connectivity, protocol, system

### 1. Public header
- **HAL:** `components/blusys/include/blusys/<module>.h`
- **Services:** `components/blusys_services/include/blusys/<category>/<module>.h`
- Opaque handle: `typedef struct blusys_<module> blusys_<module>_t;`
- All functions return `blusys_err_t`
- No ESP-IDF types

### 2. Implementation
- **HAL:** `components/blusys/src/common/<module>.c`
- **Services:** `components/blusys_services/src/<category>/<module>.c`
- Filename is `<module>.c` (no `blusys_` prefix)
- `#include "soc/soc_caps.h"` + SOC guard if not universally supported
- Use `blusys_translate_esp_err()`, `blusys_lock_*`, `calloc`/`free` pattern
- Internal utilities: `#include "blusys/internal/blusys_lock.h"` (etc.)
- Always check the return value of `blusys_lock_take()` — even in `close()` and cleanup paths
- See `temp_sensor.c` (simple HAL), `uart.c` (async/callback HAL), or `wifi.c` (event-group service) as reference

### 3. Register source and dependencies
- **HAL:** Add to `blusys_sources` in `components/blusys/CMakeLists.txt`
- **Services:** Add to `blusys_services_sources` in `components/blusys_services/CMakeLists.txt`
- Add any new ESP-IDF component to `REQUIRES` in the appropriate CMakeLists.txt

### 4. Feature flag — `components/blusys/include/blusys/target.h`
- Add `BLUSYS_FEATURE_<MODULE>` to `blusys_feature_t` enum, before `BLUSYS_FEATURE_COUNT`
- Feature flags always go in the HAL component (they describe hardware capability)

### 5. Capability mask — `components/blusys/include/blusys/internal/blusys_target_caps.h`
- Add `#define BLUSYS_<MODULE>_FEATURE_MASK BLUSYS_FEATURE_MASK(BLUSYS_FEATURE_<MODULE>)`
- Add to `BLUSYS_BASE_FEATURE_MASK` if all three targets support it; otherwise leave separate

### 6. Per-target caps — `src/targets/esp32/target_caps.c`, `esp32c3/`, `esp32s3/`
- Add the new mask to `.feature_mask` in whichever targets support the module

### 7. Umbrella header
- **HAL:** Add to `components/blusys/include/blusys/blusys.h`
- **Services:** Add to `components/blusys_services/include/blusys/blusys_services.h`

### 8. Example — `examples/<module>_basic/`
- Follow the structure described in the Examples section above
- **HAL examples:** `REQUIRES blusys` in `main/CMakeLists.txt`
- **Service examples:** `REQUIRES blusys_services` in `main/CMakeLists.txt`
- `Kconfig.projbuild` belongs in `main/`, not the project root

### 9. Docs
- `docs/modules/<module>.md` — API reference. Use the structured format: one-line description, tip admonition linking to the guide, Target Support table, Types (full typedef code blocks), Functions (signatures + parameters + returns), Lifecycle, Thread Safety, Limitations. See `docs/modules/wifi.md` as the style reference.
- `docs/guides/<module>-basic.md` — task guide. Follow Problem Statement → Prerequisites → Minimal Example → APIs Used → Expected Behavior → Common Mistakes → Example App → API Reference link. See `docs/guides/temp-sensor-basic.md` as reference.
- Add both to `mkdocs.yml` nav under the appropriate peripheral category in both Guides and API Reference tabs; run `mkdocs build --strict` to verify

### 10. PROGRESS.md
- Add module to Recent Work, Public API list, and Validation Snapshot
