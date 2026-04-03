# Getting Started

## What Blusys Is

Blusys HAL is a simplified C HAL for ESP32 devices on top of ESP-IDF v5.5.

The first release focuses on a common API for:
- ESP32-C3
- ESP32-S3

## What To Read First

1. `vision.md`
2. `architecture.md`
3. `api-design-rules.md`
4. `modules/v1-core.md`

## Set Up ESP-IDF

If `export.sh` looks for a missing `idf5.5_py3.14_env`, set the installed Python env first:

```sh
export IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.10_env
```

Then export ESP-IDF:

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
```

## Build, Flash, And Monitor One Example

The fastest first check is the smoke example.

Build for `esp32c3`:

```sh
idf.py -C examples/smoke -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 set-target esp32c3 build
```

Flash and monitor it:

```sh
idf.py -C examples/smoke -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 -p /dev/ttyUSB0 flash monitor
```

Build for `esp32s3`:

```sh
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 set-target esp32s3 build
```

Flash and monitor it:

```sh
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 -p /dev/ttyUSB0 flash monitor
```

Exit the serial monitor with `Ctrl+]`.

## More Example Guides

- `guides/system-info.md`
- `guides/gpio-basic.md`
- `guides/uart-loopback.md`
- `guides/i2c-scan.md`
- `guides/spi-loopback.md`
- `guides/pwm-basic.md`
- `guides/hardware-smoke-tests.md`

## Initial Implementation Priorities

The project starts implementation in this order:

1. foundation
2. `system`
3. `gpio`
4. `uart`
5. `i2c`
6. `spi`
7. `pwm`
8. `adc`
9. `timer`

## Phase 1 Build Validation

Use the smoke app to validate that the component is discoverable and compiles for both supported targets.

## Documentation Workflow

When a module is added:

1. create the public header
2. implement the module
3. add the example
4. add the guide
5. add the API reference page
6. validate C3 and S3 builds

## Local Docs Preview

Recommended docs toolchain:

```sh
pip install -r requirements-docs.txt
mkdocs serve
```
