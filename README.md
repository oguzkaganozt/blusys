# Blusys HAL

Blusys HAL is a simplified C hardware abstraction layer for ESP32 devices on top of ESP-IDF v5.5.

It is aimed at common embedded tasks where raw ESP-IDF APIs can feel too low-level or too verbose.

## Supported Targets

- `esp32c3`
- `esp32s3`

## Current Module Set

- foundation: `version`, `error`, `target`
- system: `system`
- digital IO: `gpio`
- communication: `uart`, `i2c`, `spi`
- timing and analog: `pwm`, `adc`, `timer`

Current async support includes:
- GPIO interrupt callbacks
- UART async TX and RX callbacks
- timer callbacks

## What You Get

- a smaller public C API than ESP-IDF
- one common API surface across ESP32-C3 and ESP32-S3 for v1
- buildable examples for each shipped module
- task-first documentation with guides and module reference pages

## Documentation

- GitHub Pages: `https://oguzkaganozt.github.io/blusys/`
- Docs index in the repo: `docs/index.md`
- Getting started guide: `docs/guides/getting-started.md`
- Hardware smoke test guide: `docs/guides/hardware-smoke-tests.md`

## Quick Start

If `export.sh` looks for a missing `idf5.5_py3.14_env`, set the installed Python env first:

```sh
export IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.10_env
```

Export ESP-IDF:

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
```

Build the smoke example for `esp32c3`:

```sh
idf.py -C examples/smoke -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 set-target esp32c3 build
```

Flash and monitor it:

```sh
idf.py -C examples/smoke -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 -p /dev/ttyUSB0 flash monitor
```

Exit the serial monitor with `Ctrl+]`.

For `esp32s3`, use the matching example config:

```sh
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 set-target esp32s3 build
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 -p /dev/ttyUSB0 flash monitor
```

## Examples

The repository includes these example apps:

- `examples/smoke/`
- `examples/system_info/`
- `examples/gpio_basic/`
- `examples/gpio_interrupt/`
- `examples/uart_loopback/`
- `examples/uart_async/`
- `examples/i2c_scan/`
- `examples/spi_loopback/`
- `examples/pwm_basic/`
- `examples/adc_basic/`
- `examples/timer_basic/`

Board-specific note:
- Some examples use pins that may need to be changed for your board.
- Review each guide and adjust pins in `idf.py menuconfig` where needed.

## Local Docs Preview

Install docs dependencies:

```sh
pip install -r requirements-docs.txt
```

Serve the docs locally:

```sh
mkdocs serve
```

Build the static site:

```sh
mkdocs build --strict
```

## Repository Layout

- `components/blusys/`: main ESP-IDF component
- `components/blusys/include/blusys/`: public headers
- `examples/`: buildable example apps
- `docs/`: user guides, module reference, and project docs
- `PROGRESS.md`: implementation status against the roadmap

## Project Status

The planned v1 blocking module set is implemented.
Phase 5 async support is implemented and is currently in runtime validation and hardening.

Track detailed implementation status in `PROGRESS.md`.

## Upstream Reference

Bundled ESP-IDF reference documentation is available in `esp-idf-en-v5.5.4/`.
