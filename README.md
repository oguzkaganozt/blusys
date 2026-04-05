# Blusys HAL

Blusys HAL is a simplified C hardware abstraction layer for ESP32 devices on top of ESP-IDF v5.5.

It provides a smaller API than raw ESP-IDF for common embedded tasks while keeping one shared public surface across supported targets.

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## Included Modules

- foundation: `version`, `error`, `target`
- system: `system`
- digital IO: `gpio`
- communication: `uart`, `i2c`, `spi`
- audio: `i2s`
- timing and analog: `pwm`, `adc`, `timer`, `pcnt`, `rmt`, `touch`, `dac`
- vehicle and field bus: `twai`

Target note:
- `pcnt` is currently available on `esp32` and `esp32s3`; `esp32c3` reports it as unsupported
- `touch` is currently available on `esp32` and `esp32s3`; `esp32c3` reports it as unsupported
- `dac` is currently available on `esp32`; `esp32c3` and `esp32s3` report it as unsupported

Async support includes:
- GPIO interrupt callbacks
- UART async TX and RX callbacks
- timer callbacks
- PCNT watch point callbacks

## Quick Start

Export ESP-IDF:

```sh
ls ~/.espressif/python_env/
export IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/<your-idf-env>
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
```

Build one example:

```sh
./build.sh examples/smoke esp32s3
```

Flash and monitor it:

```sh
./run.sh examples/smoke /dev/ttyACM0 esp32s3
```

The build target must match the connected board.

## Documentation

- Docs site: `https://oguzkaganozt.github.io/blusys/`
- Changelog: `CHANGELOG.md`
- Overview: `docs/index.md`
- Getting started: `docs/guides/getting-started.md`
- Examples and task guides: `docs/guides/`
- API reference: `docs/modules/`
- Contributor guidance: `docs/contributing.md`

## Examples

- `examples/smoke/`
- `examples/system_info/`
- `examples/gpio_basic/`
- `examples/gpio_interrupt/`
- `examples/uart_loopback/`
- `examples/uart_async/`
- `examples/i2c_scan/`
- `examples/i2s_basic/`
- `examples/spi_loopback/`
- `examples/pwm_basic/`
- `examples/adc_basic/`
- `examples/dac_basic/`
- `examples/timer_basic/`
- `examples/pcnt_basic/`
- `examples/rmt_basic/`
- `examples/twai_basic/`
- `examples/touch_basic/`
- `examples/concurrency_i2c/`
- `examples/concurrency_spi/`
- `examples/concurrency_timer/`
- `examples/concurrency_uart/`

## Project Status

`v1.0.0` is released.
The planned v1 module set is implemented and validated on the supported targets.
`V2` work has started with `pcnt`, `rmt`, `twai`, `i2s`, `touch`, and `dac`.

Detailed implementation tracking remains in `PROGRESS.md`.
