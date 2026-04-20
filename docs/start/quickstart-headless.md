# Headless Quickstart

Build a minimal headless product using the manifest-first starter.

## Create the project

```bash
blusys create --interface headless my_sensor
cd my_sensor
```

The starter keeps the first scaffold intentionally thin:

```text
my_sensor/
  blusys.project.yml
  main/
    app_main.cpp
```

`blusys.project.yml` declares the product shape: `schema`, `interface`, `capabilities`, `profile`, and `policies`. `main/app_main.cpp` owns `State`, `Action`, `update(ctx, state, action)`, and the headless runtime hooks. There is no `ui/` directory unless the product actually needs one.

## Run on host

```bash
blusys host-build my_sensor
```

The app runs in the terminal. No hardware needed.

## Build for a target

```bash
blusys build my_sensor esp32
blusys flash my_sensor /dev/ttyUSB0 esp32
blusys monitor my_sensor /dev/ttyUSB0 esp32
```

## When to expand

- Add `capabilities` in the manifest when the device needs connectivity, telemetry, OTA, storage, or diagnostics.
- Add `flows` when headless behavior has distinct operating phases.
- Add `profile` when you want a specific host or device target setup.
- Split code out of `app_main.cpp` only when the file stops being readable.

## Next steps

- [Product shape](product-shape.md) — capabilities and policies for connected products
- [Reducer Model](../app/reducer-model.md) — understand state, actions, and `update()`
- [Capabilities](../app/capabilities.md) — add WiFi, SNTP, and storage
- [Profiles](../app/profiles.md) — understand headless and device profiles
