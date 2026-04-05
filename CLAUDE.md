# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Blusys HAL** is a simplified C Hardware Abstraction Layer for ESP32 devices, built on top of ESP-IDF v5.5.4. It exposes a smaller, more stable API surface than raw ESP-IDF. Supported targets: **ESP32**, **ESP32-C3**, and **ESP32-S3**.

Current state: `v1.0.0` released — core peripherals and minimum async support are implemented and hardware-validated.

## Build and Flash Commands

All scripts live at the repo root. Default target is `esp32s3`.

```bash
# Configure (opens menuconfig)
./configure.sh examples/<name> [esp32c3|esp32s3]

# Build
./build.sh examples/<name> [esp32c3|esp32s3]

# Flash to device
./flash.sh examples/<name> <port> [esp32c3|esp32s3]
# port example: /dev/ttyACM0 or /dev/ttyUSB0

# Serial monitor (exit with Ctrl+])
./monitor.sh examples/<name> <port> [esp32c3|esp32s3]

# Full workflow: build → flash → monitor
./run.sh examples/<name> <port> [esp32c3|esp32s3]
```

`idf-common.sh` is sourced by all scripts and sets up the ESP-IDF environment:
- IDF path: `~/.espressif/v5.5.4/esp-idf/`
- Python env: `~/.espressif/python_env/idf5.5_py3.12_env`

There is no automated test runner — validation is done via hardware smoke tests on physical devices. See `docs/guides/hardware-smoke-tests.md`.

## Documentation

```bash
pip install -r requirements-docs.txt
mkdocs serve    # local preview
mkdocs build    # static site
```

Docs live in `docs/`. Key files: `docs/architecture.md`, `docs/api-design-rules.md`, `docs/roadmap.md`.

## Code Architecture

### Layer Model

```
Application
  → Blusys Public API  (components/blusys/include/blusys/*.h)
  → Module Implementation  (components/blusys/src/common/*.c)
  → Internal / Port Layer  (components/blusys/src/internal/)
  → Target-Specific Code   (components/blusys/src/targets/esp32c3/ or esp32s3/)
  → ESP-IDF Drivers
  → ESP32 Hardware
```

### Component Structure

```
components/blusys/
├── include/blusys/        # Public headers — the full API surface
│   ├── error.h            # blusys_err_t and error utilities
│   ├── gpio.h             # GPIO (stateless, pin-based)
│   ├── uart.h             # UART (handle-based, sync + async)
│   ├── i2c.h              # I2C master (handle-based)
│   ├── spi.h              # SPI master (handle-based)
│   ├── pwm.h              # PWM (handle-based)
│   ├── adc.h              # ADC (handle-based)
│   ├── timer.h            # Hardware timers with callbacks (handle-based)
│   ├── system.h           # Uptime, heap, reset reason, restart
│   ├── target.h           # Target ID and feature detection
│   └── version.h          # Version constants
└── src/
    ├── common/            # One .c file per module (platform-agnostic logic)
    ├── internal/          # Private headers: locking, timeout, esp_err mapping
    └── targets/
        ├── esp32c3/       # Target-specific implementations
        └── esp32s3/
```

### Key API Conventions

- All return values use `blusys_err_t` (never `esp_err_t` in public API).
- Stateful peripherals (UART, I2C, SPI, PWM, ADC, Timer) use an opaque handle returned by `blusys_<module>_open()` and released with `blusys_<module>_close()`.
- GPIO is stateless and pin-based — no handle.
- Timeout parameter: pass `BLUSYS_TIMEOUT_FOREVER` (`-1`) to block indefinitely.
- No ESP-IDF types, handles, or macros appear in public headers.
- No `#ifdef` for target differences in public or common code — use capability queries (`blusys_target_supports(feature)`) or the target layer.

### Internal Utilities

- `blusys_lock.h` / `blusys_lock_freertos.c` — FreeRTOS mutex abstraction used by all stateful modules.
- `blusys_esp_err.h` — Maps `esp_err_t` → `blusys_err_t`.
- `blusys_timeout.h` — Timeout value conversion helpers.
- `blusys_target_caps.h` — Per-target feature capability table.

### Examples

Each example in `examples/` is a standalone ESP-IDF project that references the blusys component via `EXTRA_COMPONENT_DIRS`. Each has:
- `main/<name>_main.c` — application entry point
- `sdkconfig.esp32c3` and `sdkconfig.esp32s3` — per-target SDK configs
- `build-esp32c3/` and `build-esp32s3/` — build artifact directories (gitignored pattern)

The example CMakeLists.txt resolves the repo root relative to the example directory and adds `components/` to the component search path.

## Adding a New Module

1. Add public header to `components/blusys/include/blusys/<module>.h` — use only `blusys_err_t` and opaque handles.
2. Add implementation to `components/blusys/src/common/blusys_<module>.c`.
3. Add any target-specific code under `src/targets/esp32c3/` and `src/targets/esp32s3/`.
4. Register the new `.c` file in `components/blusys/CMakeLists.txt` under `SRCS`.
5. Add required ESP-IDF driver to `REQUIRES` in the same CMakeLists.txt.
6. Create a matching example in `examples/<module>_basic/`.
