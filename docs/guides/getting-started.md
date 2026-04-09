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
mkdir ~/my_product && cd ~/my_product
blusys create --starter interactive
```

`blusys create` scaffolds a four-CMakeLists product layout in the current
directory:

```text
my_product/
  CMakeLists.txt              # top-level — derives BLUSYS_BUILD_UI from starter type
  sdkconfig.defaults
  main/
    CMakeLists.txt            # REQUIRES app blusys_framework
    app_main.cpp              # extern "C" void app_main(void) — always .cpp
    idf_component.yml         # managed deps for blusys / blusys_services /
                              # blusys_framework (+ lvgl/lvgl for interactive)
  app/
    CMakeLists.txt            # gates ui/ui_screens/ui_widgets on BLUSYS_BUILD_UI
    product_config.cmake      # BLUSYS_PRODUCT_NAME / BLUSYS_STARTER_TYPE / BLUSYS_TARGETS
    controllers/
      home_controller.hpp     # sample controller
      home_controller.cpp
    integrations/
    config/
    ui/                       # interactive only
      theme_init.{hpp,cpp}    # populates blusys::ui::theme_tokens at boot
      screens/
        main_screen.{hpp,cpp} # sample [-]/[+] counter screen on the widget kit
```

The two starter types:

- **`--starter interactive`** (default) — the framework UI tier is built and
  the scaffold ships a sample screen, theme initialization, and an
  `lvgl/lvgl` managed dependency.
- **`--starter headless`** — the framework UI tier is excluded entirely
  (`BLUSYS_BUILD_UI=OFF`); the scaffold ships a controller-only sample. Use
  this for industrial / non-display products that still want the framework
  controller / intent / feedback spine.

Edit `app/product_config.cmake` to change the product name, starter type, or
supported targets after scaffolding. Platform components are pulled by the
ESP-IDF managed component manager from `main/idf_component.yml` — never via
`EXTRA_COMPONENT_DIRS`.

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

## ESP-IDF Documentation

The ESP-IDF offline docs are included with your ESP-IDF installation at:

```
~/.espressif/frameworks/esp-idf-v5.5.4/docs/en/
```

Online docs are available at [docs.espressif.com/projects/esp-idf/en/v5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/).

## Next Steps

- read a task guide in `guides/`
- use `modules/` for API details
