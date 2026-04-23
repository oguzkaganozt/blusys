# Headless Quickstart

Build a **minimal `headless` app** (terminal on host, no `ui/` by default). The only scaffold difference from [Interactive quickstart](quickstart-interactive.md) is `--interface headless` and no `ui/` folder.

!!! tip
    Manifest vocabulary is in [Product shape](product-shape.md). This page is the shortest path to a running headless binary.

## Create the project

**Headless** = `--interface headless`; no `ui/` unless you add one later.

```bash
blusys create --interface headless my_sensor
cd my_sensor
```

??? note "Generated layout"
    The starter is intentionally thin:

    ```text
    my_sensor/
      blusys.project.yml
      main/
        app_main.cpp
    ```

`main/app_main.cpp` holds `State`, `Action`, `update()`, and headless hooks.

## Run on host

```bash
blusys host-build
```

The app runs in the terminal. No hardware required.

???+ note "Build / flash a physical device (optional)"
    ```bash
    blusys build esp32
    blusys flash /dev/ttyUSB0 esp32
    blusys monitor /dev/ttyUSB0 esp32
    ```

    Adjust chip and serial port for your board; you need ESP-IDF / `blusys` [environment](https://github.com/oguzkaganozt/blusys/blob/main/README.md) from the README.

## When to expand

- Add **`capabilities`** for connectivity, telemetry, OTA, storage, or diagnostics.
- Add **`profile`** for a specific host or device target.
- Split `app_main.cpp` when it stops being readable.

## Next steps

- [Product shape](product-shape.md) — connected product shape
- [Reducer model](../app/reducer-model.md) — state, actions, `update()`
- [Capabilities](../app/capabilities.md) — WiFi, SNTP, storage
- [Profiles](../app/profiles.md) — headless and device profiles
