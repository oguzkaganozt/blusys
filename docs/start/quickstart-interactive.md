# Interactive Quickstart

Use this when the product has a local UI and you want to iterate on the host first.

!!! tip
    This is the shortest path to a screenful binary. The same reducer path later runs on device.

## Create the project

```bash
blusys create --interface interactive my_product
cd my_product
```

??? note "Generated layout"
    ```text
    my_product/
      blusys.project.yml
      main/
        app_main.cpp
        ui/
          app_ui.cpp
      host/
    ```

    `main/app_main.cpp` holds the product wiring, reducer, and runtime hooks.

## Run on host

```bash
blusys host-build
```

The app opens an SDL2 window with the default theme. Arrow keys move focus; Enter activates.

## Optional: build for a device

```bash
blusys build . esp32s3
blusys flash /dev/ttyUSB0 esp32s3
blusys monitor /dev/ttyUSB0 esp32s3
```

Adjust chip and serial port for your board.

## When to expand

- Add **capabilities** for connectivity, storage, telemetry, OTA, or diagnostics.
- Add a **profile** for the display or hardware preset you actually ship.
- Split `app_main.cpp` when it stops being readable.

## Next steps

- [Product shape](product-shape.md)
- [Reducer model](../app/reducer-model.md)
- [Views & widgets](../app/views-and-widgets.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)
