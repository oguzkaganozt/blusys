# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Blusys HAL** is a simplified C Hardware Abstraction Layer for ESP32 devices, built on top of ESP-IDF v5.5.4. It exposes a smaller, more stable API surface than raw ESP-IDF. Supported targets: **ESP32**, **ESP32-C3**, and **ESP32-S3**. Current release: `v2.0.0`.

## Build and Flash Commands

All scripts live at the repo root. Default target is `esp32s3`.

```bash
./configure.sh examples/<name> [esp32|esp32c3|esp32s3]   # opens menuconfig
./build.sh     examples/<name> [esp32|esp32c3|esp32s3]   # build only
./flash.sh     examples/<name> <port> [esp32|esp32c3|esp32s3]
./monitor.sh   examples/<name> <port> [esp32|esp32c3|esp32s3]
./run.sh       examples/<name> <port> [esp32|esp32c3|esp32s3]   # build+flash+monitor
```

`idf-common.sh` is sourced by all scripts:
- IDF path: `~/.espressif/v5.5.4/esp-idf/`
- Python env: `~/.espressif/python_env/idf5.5_py3.12_env`

Build artifacts land in `examples/<name>/build-esp32/`, `build-esp32c3/`, `build-esp32s3/`. If Kconfig values change, delete the build directory before rebuilding — stale `sdkconfig` will silently miss new `CONFIG_*` symbols.

There is no automated test runner. Validation is via hardware smoke tests: `docs/guides/hardware-smoke-tests.md`.

## Documentation

```bash
pip install -r requirements-docs.txt
mkdocs serve          # local preview
mkdocs build --strict # must pass before any merge
```

`mkdocs build --strict` is the doc gate — it fails on broken nav references or missing pages.

## Code Architecture

### Layer Model

```
Application
  → Blusys Public API     (components/blusys/include/blusys/*.h)
  → Module Implementation (components/blusys/src/common/*.c)
  → Internal Utilities    (components/blusys/src/internal/)
  → Target Capabilities   (components/blusys/src/targets/esp32*/target_caps.c)
  → ESP-IDF Drivers
  → Hardware
```

### Key API Conventions

- All return values use `blusys_err_t` — never `esp_err_t` in public headers.
- Stateful peripherals use an opaque handle: `blusys_<module>_open()` → `blusys_<module>_close()`.
- GPIO is stateless and pin-based (no handle).
- Timeout parameter: `BLUSYS_TIMEOUT_FOREVER` (`-1`) to block indefinitely.
- No ESP-IDF types, handles, or macros in public headers.
- No `#ifdef` for target differences in public or common code — use `blusys_target_supports(feature)` or the SOC guard pattern (see below).

### Internal Utilities (`src/internal/`)

- `blusys_lock.h` / `blusys_lock_freertos.c` — FreeRTOS mutex used by all stateful modules.
- `blusys_esp_err.h` — `blusys_translate_esp_err(esp_err_t)` maps ESP errors to `blusys_err_t`.
- `blusys_timeout.h` — timeout conversion helpers.
- `blusys_target_caps.h` — `BLUSYS_FEATURE_MASK(feature)` macro and `BLUSYS_BASE_FEATURE_MASK`; per-target feature bitmasks.

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

### Examples

Each example is a standalone ESP-IDF project:
- `main/<name>_main.c` — entry point
- `main/CMakeLists.txt` — `idf_component_register(SRCS "..." REQUIRES blusys)`
- `main/Kconfig.projbuild` — menuconfig options (must be in `main/`, not project root)
- `CMakeLists.txt` — sets `EXTRA_COMPONENT_DIRS` to `../../components`
- `sdkconfig.esp32c3` / `sdkconfig.esp32s3` — optional per-target defaults

## Adding a New Module

### 1. Public header — `include/blusys/<module>.h`
- Opaque handle: `typedef struct blusys_<module> blusys_<module>_t;`
- All functions return `blusys_err_t`
- No ESP-IDF types

### 2. Implementation — `src/common/<module>.c`
- Filename is `<module>.c` (no `blusys_` prefix)
- `#include "soc/soc_caps.h"` + SOC guard if not universally supported
- Use `blusys_translate_esp_err()`, `blusys_lock_*`, `calloc`/`free` pattern
- See `temp_sensor.c` (simple) or `uart.c` (async/callback) as reference

### 3. Register source and dependencies — `CMakeLists.txt`
- Add `src/common/<module>.c` to `blusys_sources`
- Add any new ESP-IDF component to `REQUIRES`

### 4. Feature flag — `include/blusys/target.h`
- Add `BLUSYS_FEATURE_<MODULE>` to `blusys_feature_t` enum, before `BLUSYS_FEATURE_COUNT`

### 5. Capability mask — `src/internal/blusys_target_caps.h`
- Add `#define BLUSYS_<MODULE>_FEATURE_MASK BLUSYS_FEATURE_MASK(BLUSYS_FEATURE_<MODULE>)`
- Add to `BLUSYS_BASE_FEATURE_MASK` if all three targets support it; otherwise leave separate

### 6. Per-target caps — `src/targets/esp32/target_caps.c`, `esp32c3/`, `esp32s3/`
- Add the new mask to `.feature_mask` in whichever targets support the module

### 7. Umbrella header — `include/blusys/blusys.h`
- Add `#include "blusys/<module>.h"` in alphabetical order

### 8. Example — `examples/<module>_basic/`
- Follow the structure described in the Examples section above
- `Kconfig.projbuild` belongs in `main/`, not the project root

### 9. Docs
- `docs/modules/<module>.md` — API reference (see `docs/modules/temp_sensor.md` as style reference)
- `docs/guides/<module>-basic.md` — task guide (see `docs/guides/temp-sensor-basic.md`)
- Add both to `mkdocs.yml` under Guides and API Reference; run `mkdocs build --strict` to verify

### 10. PROGRESS.md
- Add module to Recent Work, Public API list, and Validation Snapshot
