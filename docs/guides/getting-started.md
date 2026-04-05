# Getting Started

## Goal

Build and run one Blusys example on your board.

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## 1. Export ESP-IDF

If `export.sh` looks for a missing ESP-IDF Python env, first check which one exists:

```sh
ls ~/.espressif/python_env/
```

Then export ESP-IDF:

```sh
export IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/<your-idf-env>
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
```

On this machine the installed env is usually `idf5.5_py3.12_env`.

## 2. Build The Smoke Example

Using the helper script:

```sh
./build.sh examples/smoke esp32s3
```

Or directly with `idf.py`:

```sh
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 set-target esp32s3 build
```

## 3. Find The Serial Port

On Linux:

```sh
ls /dev/ttyACM* /dev/ttyUSB*
```

## 4. Flash And Monitor

Using the helper script:

```sh
./run.sh examples/smoke /dev/ttyACM0 esp32s3
```

Or directly with `idf.py`:

```sh
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 -p /dev/ttyACM0 flash monitor
```

Exit the serial monitor with `Ctrl+]`.

## 5. Check The Output

Expected output includes lines such as:

- `Blusys smoke app`
- `target: ESP32-S3`
- `feature_gpio: yes`

## Important Notes

- the selected build target must match the connected board
- some examples use configurable pins, so review `menuconfig` when board defaults are not safe
- if a flash fails because of a target mismatch, rebuild for the correct chip

## Next Steps

- read a task guide in `guides/`
- use `guides/hardware-smoke-tests.md` for wider board validation
- use `modules/` for API details
