# Getting Started

## Goal

Install blusys and build your first project.

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## Prerequisites

Install ESP-IDF v5.5+ following the [official guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/).

The standard install places ESP-IDF in `~/.espressif/` which blusys auto-detects.

## 1. Install Blusys

```sh
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
~/.blusys/install.sh
```

Verify the installation:

```sh
blusys version
```

This should show the blusys version and detected ESP-IDF path.

## 2. Create A Project

```sh
mkdir ~/my_project && cd ~/my_project
blusys create
```

This scaffolds `CMakeLists.txt`, `main/app_main.c`, and other project files in the current directory.

## 3. Build

```sh
blusys build esp32s3
```

## 4. Find The Serial Port

```sh
ls /dev/ttyACM* /dev/ttyUSB*
```

## 5. Flash And Monitor

```sh
blusys run /dev/ttyACM0 esp32s3
```

Exit the serial monitor with `Ctrl+]`.

## 6. Run An Example

Examples are bundled with blusys:

```sh
blusys example                                   # list all examples
blusys example smoke build esp32s3               # build the smoke example
blusys example smoke run /dev/ttyACM0 esp32s3    # build, flash, and monitor
```

## Updating Blusys

```sh
blusys update
```

## Important Notes

- the selected build target must match the connected board
- blusys requires ESP-IDF v5.5 or later
- run `blusys version` to check your detected ESP-IDF
- if ESP-IDF is not auto-detected, set `export IDF_PATH=/path/to/esp-idf`

## Next Steps

- read a task guide in `guides/`
- use `modules/` for API details
