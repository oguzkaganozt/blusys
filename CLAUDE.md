# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Read First

Before changing public API, docs, or project status, read these files — they're the canonical sources and may be more up to date than this guide:

- `PROGRESS.md` — current phase, module inventory, active work
- `platform-transition/` — locked planning docs for the in-progress V6 platform transition
- `AGENTS.md` — repo conventions, CLI quirks, verification info
- `docs/architecture.md` — current repo architecture and tiering
- `docs/guidelines.md` — public API design rules and contribution workflow

Trust executable sources over prose: the `blusys` shell script, `components/*/CMakeLists.txt`, example `idf_component.yml` files, and `.github/workflows/*.yml`. Some prose inventories may be older than the code.

## Project Overview

**Blusys** is an ESP32 platform repo built on top of ESP-IDF v5.5.4. Supported targets: **ESP32**, **ESP32-C3**, **ESP32-S3**. Released state is still `v5.0.0`, but the repo is now actively implementing the V6 transition branch.

## Active Transition (V5 → V6)

Planning is **locked** in `platform-transition/`, and implementation is now in progress on the `platform-transition` branch.

Locked decisions future sessions need to know:

- **Three tiers**, not two: `blusys` (HAL + drivers, C) → `blusys_services` (C) → `blusys_framework` (C++).
- **Drivers** (display/input/sensor/actuator: lcd, led_strip, seven_seg, button, encoder, dht, buzzer) now live under `components/blusys/src/drivers/<category>/`. HAL/drivers boundary inside `components/blusys/` is enforced by directory discipline + `blusys lint`.
- **`usb_hid` stays in services.** It is runtime orchestration across USB host and BLE, not a simple driver.
- **Framework is the only C++ tier in V1.** Services migration to C++ is deferred to V2.
- **`blusys_framework` ships the V1 surface in full:** core spine (`router`, `intent`, `feedback`, `controller`, `runtime`), layout primitives (`screen`, `row`, `col`, `label`, `divider`), the **V1 widget kit** (`bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`), encoder focus helpers (`create_encoder_group`, `auto_focus_screen`), the `theme_tokens` registry, the semantic callbacks header (`callbacks.hpp`), and the `widget-author-guide.md` codifying the six-rule contract every widget follows. `bu_knob` is deferred to V2.
- **Widget kit** built on LVGL with a six-rule component contract; theme is a single C++ struct populated at boot. Each Camp 2 widget uses a fixed-capacity slot pool keyed by `BLUSYS_UI_<NAME>_POOL_SIZE` for callback storage (default 32 for button/toggle/slider, 8 for modal/overlay). No JSON, no Python, no design-tool integration.
- **Product scaffold** generates four CMakeLists files (top-level + `main/` + `app/` + `app/product_config.cmake`). `app/` becomes its own ESP-IDF component. Platform components are pulled via `main/idf_component.yml` managed dependencies — never via `EXTRA_COMPONENT_DIRS = "$ENV{BLUSYS_PATH}/components"` (that's the monorepo internal pattern only).
- **Logging** in framework code goes through a thin `blusys/log.h` wrapper (`BLUSYS_LOGE/I/W/D`). HAL and services keep using `esp_log.h` directly.
- **`blusys_all.h` was removed in Phase 8.** Code uses the per-tier umbrella headers (`blusys/blusys.h`, `blusys/blusys_services.h`, `blusys/framework/framework.hpp`); driver-specific headers (`blusys/drivers/<category>/<module>.h`) are also acceptable when an example only needs one driver.

Completed so far:

- Phase 2: identity/docs alignment
- Phase 3: drivers move + framework component stub + CI lint + repo/docs rewrite
- Phase 4: framework C++ compile policy + `blusys/log.h` + foundational framework headers
- Phase 4.5: PC + SDL2 host harness under `scripts/host/` (LVGL v9.2.2 via FetchContent, `blusys host-build` CLI subcommand)
- Phase 5: V1 widget kit (`bu_button`/`bu_toggle`/`bu_slider`/`bu_modal`/`bu_overlay`), `widget-author-guide.md`, `ui/input/` encoder helpers — `bu_knob` deferred to V2
- Phase 6: framework core spine end-to-end, validated by `examples/framework_app_basic/` (button on_press → runtime.post_intent → controller.handle → slider_set_value / submit_route → ui_route_sink → overlay_show)
- Phase 7: `blusys create --starter <headless|interactive>` generates the four-CMakeLists product scaffold (top-level + `main/` + `app/` + `app/product_config.cmake`), `main/app_main.cpp` (always `.cpp`, no `blusys_all.h`), `main/idf_component.yml` pinning all three platform components to `v6.0.0` (+ `lvgl/lvgl ~9.2` for interactive), a sample `home_controller`, and (interactive only) `app/ui/theme_init` + `app/ui/screens/main_screen` building a `[-]/[+]` counter on the widget kit. Both starter types build clean against the local checkout via `override_path`. Spec correction: `EXTRA_COMPONENT_DIRS` points at `${CMAKE_CURRENT_LIST_DIR}/app`, not the project root, because ESP-IDF's `__project_component_dir` treats a path with a `CMakeLists.txt` as a component itself — pointing at the project root recursively re-includes the top-level CMakeLists during requirement scanning.
- Phase 8: example ecosystem migrated to per-tier umbrella headers (1 example + 12 guides swept), `components/blusys_services/include/blusys/blusys_all.h` deleted, prose mentions in `docs/architecture.md` and `docs/guides/framework.md` updated. Fourth framework example added: `examples/framework_encoder_basic/` wires a real `lv_indev_t` of type `LV_INDEV_TYPE_ENCODER` into `create_encoder_group` + `auto_focus_screen` and bridges the `blusys_encoder` driver to LVGL via a callback-to-poll state buffer; uses a stub display so it builds and runs on all three targets without LCD hardware. Pre-Phase-8 housekeeping also wired `scripts/lint-layering.sh` into CI as a Tier-0 gate and extended the regex to catch angle-bracket includes.

Next planned step: Phase 9 — validation chain (PC + LVGL + SDL2 → ESP-IDF QEMU → real hardware), then tag `v6.0.0`.

Full plan, decisions log, and rationale: `platform-transition/`. Current phase status: `PROGRESS.md`.

## Build and Flash Commands

All commands go through the `blusys` CLI. Default target is `esp32s3`. All commands default to the current directory when no project is specified.

```bash
blusys create [--starter <headless|interactive>] [path]           # scaffold product (default starter: interactive)
blusys build          [project] [esp32|esp32c3|esp32s3]
blusys flash          [project] [port] [esp32|esp32c3|esp32s3]
blusys monitor        [project] [port] [esp32|esp32c3|esp32s3]
blusys run            [project] [port] [esp32|esp32c3|esp32s3]    # build + flash + monitor
blusys config         [project] [esp32|esp32c3|esp32s3]           # opens menuconfig
blusys menuconfig     [project] [esp32|esp32c3|esp32s3]           # alias for config
blusys size           [project] [esp32|esp32c3|esp32s3]           # firmware size breakdown
blusys erase          [project] [port] [esp32|esp32c3|esp32s3]    # erase entire flash
blusys clean          [project] [esp32|esp32c3|esp32s3]           # remove one target build dir
blusys fullclean      [project]                                   # remove all target build dirs
blusys build-examples                                             # build all examples × all targets
blusys host-build                                                 # build the PC + SDL2 host harness (scripts/host/)
blusys example <name> [build|flash|monitor|run] [port] [target]   # operate on bundled examples
blusys qemu           [project] [esp32|esp32c3|esp32s3]           # build + run in QEMU
blusys install-qemu                                               # fetch pre-built Espressif QEMU
blusys config-idf                                                 # select default ESP-IDF version
blusys version                                                    # show blusys version + IDF info
blusys update                                                     # self-update via git pull
```

`blusys` configuration:
- IDF detection: auto-detects from `$IDF_PATH`, PATH, or `~/.espressif/*/esp-idf/`
- `BLUSYS_PATH` is exported automatically and used by example/scaffold project CMakeLists.txt

### CLI quirks worth knowing

- **Serial auto-detect** only works when exactly one `/dev/ttyUSB*` or `/dev/ttyACM*` device is connected. Otherwise pass the port explicitly.
- **`set-target` only runs on first configure.** If you change targets, edit Kconfig, or change `sdkconfig.defaults`, delete the matching `build-<target>/` directory before rebuilding — stale `sdkconfig` will silently miss new `CONFIG_*` symbols.
- Build artifacts land in `examples/<name>/build-esp32/`, `build-esp32c3/`, `build-esp32s3/`.
- Both `sdkconfig.defaults` and `sdkconfig.<target>` are merged via `-DSDKCONFIG_DEFAULTS` (semicolon-separated when both exist).
- **QEMU networking is OpenCores Ethernet emulation, not WiFi.** WiFi-dependent examples (`wifi_connect`, `mqtt_basic`, etc.) won't run in QEMU.
- Use raw `idf.py` only when matching CI behavior or debugging the CLI itself; for normal work always go through `blusys`.

## Validation Gates

There is no repo-local unit test or static analysis pipeline. Validation happens at four levels:

1. **Compile gate (CI):** `.github/workflows/ci.yml` builds every example for all three targets. Local equivalent: `blusys build-examples`.
2. **QEMU smoke gate (CI):** after the build gate, six examples run in QEMU on all three targets — `smoke`, `system_info`, `timer_basic`, `nvs_basic`, `wdt_basic`, `concurrency_timer`. Local equivalent: `blusys install-qemu` then `blusys qemu <example> <target>`. The QEMU runner script is `scripts/qemu-test.sh`.
3. **Doc gate:** `mkdocs build --strict` must pass. Fails on broken nav references, missing pages, or bad cross-references. Run before any merge.
4. **Hardware gate:** new or changed public modules need at least one real-board smoke test. See `docs/guides/hardware-smoke-tests.md` and the report template at `docs/guides/hardware-validation-report-template.md`.
5. **Host harness (Phase 4.5):** `blusys host-build` configures `scripts/host/` via CMake, fetches LVGL v9.2.2 from upstream, and builds `hello_lvgl` against SDL2. Useful for visual iteration on the framework UI without flashing hardware. Requires `libsdl2-dev` (or distro equivalent), `cmake`, and `pkg-config`. See `scripts/host/README.md`.

## Documentation

```bash
pip install -r requirements-docs.txt
mkdocs serve          # local preview
mkdocs build --strict # doc gate (run before merge)
```

The site uses MkDocs Material with navigation tabs: **Home**, **Guides**, **API Reference**, **Project**. The nav hierarchy in `mkdocs.yml` groups pages under peripheral categories. When adding pages, place them in the correct category in both the Guides and API Reference tabs. Card grid landing pages live at `docs/guides/index.md` and `docs/modules/index.md` — update them when adding a new module.

The docs/nav already use the Drivers + Services + Framework split introduced during the transition. Keep that structure consistent when adding pages.

## Code Architecture

The codebase is now split into three ESP-IDF components:

- **`components/blusys/`** — HAL + drivers. HAL sources live in `src/common/`; drivers live in `src/drivers/<category>/`.
- **`components/blusys_services/`** — runtime services. `ui` and `usb_hid` stay here along with connectivity/protocol/system services.
- **`components/blusys_framework/`** — framework tier (C++). Ships the V1 widget kit (`bu_button`/`bu_toggle`/`bu_slider`/`bu_modal`/`bu_overlay`), layout primitives, theme registry, core spine (`router`/`intent`/`feedback`/`controller`/`runtime`), encoder focus helpers, and the `widget-author-guide.md`.

Dependency direction:

- `blusys_framework` -> `blusys_services` -> `blusys`
- reverse dependencies are forbidden

```
Application
  → Framework API         (components/blusys_framework/include/blusys/framework/...)
  → Services API          (components/blusys_services/include/blusys/<category>/*.h)
  → HAL + Driver API      (components/blusys/include/blusys/*.h, components/blusys/include/blusys/drivers/<category>/*.h)
  → Services Impl         (components/blusys_services/src/<category>/*.c)
  → HAL Implementation    (components/blusys/src/common/*.c)
  → Driver Implementation (components/blusys/src/drivers/<category>/*.c)
  → Internal Utilities    (components/blusys/include/blusys/internal/)
  → Target Capabilities   (components/blusys/src/targets/esp32*/target_caps.c)
  → ESP-IDF Drivers
  → Hardware
```

**HAL modules:** gpio, gpio_expander, uart, i2c, i2c_slave, spi, spi_slave, adc, dac, pwm, mcpwm, timer, pcnt, rmt, rmt_rx, i2s, i2s_rx, sdm, twai, wdt, sleep, temp_sensor, touch, sdmmc, sd_spi, nvs, one_wire, efuse, ulp, usb_host, usb_device, system, error, target, version.

**Driver modules:** lcd, led_strip, seven_seg, button, encoder, dht, buzzer.

**Service modules** (by category):
- **display/runtime:** ui
- **input/runtime:** usb_hid
- **connectivity:** wifi, wifi_prov, wifi_mesh, espnow, bluetooth, ble_gatt, mdns
- **protocol:** mqtt, http_client, http_server, ws_client
- **system:** fs, fatfs, console, power_mgmt, sntp, ota, local_ctrl

**Umbrella headers:**
- `blusys/blusys.h` — includes HAL + driver modules
- `blusys/blusys_services.h` — includes service modules
- `blusys/framework/framework.hpp` — framework umbrella header

### Key API Conventions

- All return values use `blusys_err_t` — never `esp_err_t` in public headers.
- Stateful peripherals use an opaque handle: `blusys_<module>_open()` → `blusys_<module>_close()`.
- GPIO is stateless and pin-based (no handle).
- Timeout parameter: `BLUSYS_TIMEOUT_FOREVER` (`-1`) to block indefinitely.
- No ESP-IDF types, handles, or macros in public headers.
- No `#ifdef` for target differences in public or common code — use `blusys_target_supports(feature)` or the SOC capability guard pattern below.

Full API design rules: `docs/guidelines.md`.

### Internal Utilities (`components/blusys/include/blusys/internal/`)

These headers are public to both components but are implementation details, not user-facing API. Include them as `#include "blusys/internal/blusys_lock.h"` etc.

- `blusys_lock.h` / `blusys_lock_freertos.c` (`src/internal/`) — FreeRTOS mutex used by all stateful modules.
- `blusys_esp_err.h` — `blusys_translate_esp_err(esp_err_t)` maps ESP errors to `blusys_err_t`.
- `blusys_timeout.h` — timeout conversion helpers.
- `blusys_target_caps.h` — `BLUSYS_FEATURE_MASK(feature)` macro, `BLUSYS_BASE_FEATURE_MASK`, per-target feature bitmasks.
- `blusys_nvs_init.h` — `blusys_nvs_ensure_init()` shared helper used by any module that requires NVS (wifi, espnow, bluetooth, ble_gatt). Use this instead of calling `nvs_flash_init()` directly.

**Critical thread-safety rule:** Never hold a `blusys_lock_t` across a blocking wait (`xEventGroupWaitBits`, `vTaskDelay`, network/disk wait, long service call). The pattern is: take lock → prepare → give lock → block. Violating this deadlocks any concurrent caller on the same handle. See `wifi.c:blusys_wifi_connect()` and `mqtt.c:blusys_mqtt_connect()` as correct reference implementations.

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

**Deferred modules** (not supported on any current target):

| Module       | Reason |
|--------------|--------|
| `ana_cmpr`   | `SOC_ANA_CMPR_SUPPORTED` is false for ESP32, ESP32-C3, ESP32-S3; only available on ESP32-C5/H2/P4/C61 |

All other modules are in `BLUSYS_BASE_FEATURE_MASK` and available on all targets. Full support matrix: `docs/target-matrix.md`.

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

**Wi-Fi-gated modules:** the V3 connectivity modules (`http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`) use `SOC_WIFI_SUPPORTED` as their guard rather than a per-target `FEATURE_MASK` entry. They also require `blusys_wifi_connect()` to be called first at the application level.

**NimBLE-gated modules:** `bluetooth` and `ble_gatt` use `CONFIG_BT_NIMBLE_ENABLED` (set via menuconfig) as their guard.

**Optional managed components:** several modules detect their dependency at build time using the `if(... IN_LIST build_components)` pattern in `blusys_services/CMakeLists.txt` and define a `BLUSYS_HAS_*` macro when the component is present. Projects must declare these in their own `main/idf_component.yml`:

| Module       | Managed component         | Macro                    |
|--------------|---------------------------|--------------------------|
| `mdns`       | `espressif/mdns`          | `BLUSYS_HAS_MDNS`        |
| `ui`         | `lvgl/lvgl`               | `BLUSYS_HAS_LVGL`        |
| `usb_hid` (USB transport) | `espressif/usb_host_hid` | `BLUSYS_HAS_USB_HOST_HID` |
| `usb_device` | `espressif/esp_tinyusb`   | `BLUSYS_HAS_TINYUSB`     |

See `examples/mdns_basic/main/idf_component.yml` as reference.

### Examples

Each example is a standalone ESP-IDF project under `examples/<name>/`:
- `main/<name>_main.c` — entry point
- `main/CMakeLists.txt` — `idf_component_register(SRCS "..." REQUIRES blusys)` (or `REQUIRES blusys_services` for service modules)
- `main/Kconfig.projbuild` — menuconfig options (must live in `main/`, not the project root)
- `CMakeLists.txt` — sets `EXTRA_COMPONENT_DIRS` to `../../components`
- `sdkconfig.esp32c3` / `sdkconfig.esp32s3` — optional per-target defaults

## Adding a New Module

First decide which component the module belongs to:
- **HAL** (`components/blusys/`) — direct hardware abstraction (GPIO, bus protocols, timers, ADC, etc.)
- **Services** (`components/blusys_services/`) — higher-level modules in one of 7 categories: display, input, sensor, actuator, connectivity, protocol, system

Drivers already live in `components/blusys/src/drivers/<category>/`. `usb_hid` remains in `components/blusys_services/src/input/`.

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
- For optional managed-component dependencies, follow the existing `if(... IN_LIST build_components)` pattern (see the `mdns`/`ui`/`usb_host_hid`/`tinyusb` blocks)

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
- Add both to `mkdocs.yml` nav under the appropriate peripheral category in both Guides and API Reference tabs.
- Update card-grid landing pages: `docs/guides/index.md` and `docs/modules/index.md`.
- Run `mkdocs build --strict` to verify nav references and cross-links.
- If the support matrix changed: update `docs/target-matrix.md`.

### 10. PROGRESS.md
Add the module to the **Module Inventory** section under its component (HAL / Drivers / Services / Framework). If hardware smoke testing is pending, add it to the **Validation Snapshot** "pending hardware smoke tests" list. PROGRESS.md is currently structured as the V6 transition log; the per-version "Recent Work" lists from prior releases (V1–V5) are historical and frozen — do not append to them.
