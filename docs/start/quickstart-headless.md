# Headless Quickstart

Use this for connected products that do not need a local UI.

!!! tip
    Headless keeps the same reducer model, but skips display bring-up and local screens.

## Create the project

```bash
blusys create --interface headless my_sensor
cd my_sensor
```

??? note "Generated layout"
    ```text
    my_sensor/
      blusys.project.yml
      main/
        app_main.cpp
      host/
    ```

    `main/app_main.cpp` holds the reducer, event mapping, and headless hooks.

## Run on host

```bash
blusys host-build
```

The app runs in the terminal. No hardware is required.

## Optional: build for a device

```bash
blusys build . esp32
blusys flash /dev/ttyUSB0 esp32
blusys monitor /dev/ttyUSB0 esp32
```

Adjust chip and serial port for your board.

## When to expand

- Add **capabilities** for connectivity, telemetry, OTA, storage, or diagnostics.
- Add a **profile** only if you later move to an interactive hardware surface.
- Split `app_main.cpp` when it stops being readable.

## Next steps

- [Product shape](product-shape.md)
- [Reducer model](../app/reducer-model.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)
