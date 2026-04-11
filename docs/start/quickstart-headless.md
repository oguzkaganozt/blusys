# Headless Quickstart

Build a headless-first connected product using the same `blusys::app` reducer model.

The canonical headless starting point is the `edge node` archetype.

## Create the project

```bash
blusys create --archetype edge-node my_sensor
cd my_sensor
```

## Run on host

```bash
blusys host-build my_sensor
```

The app runs in the terminal, printing heartbeat logs. No hardware needed.

## Build for a target

```bash
blusys build my_sensor esp32
blusys flash my_sensor /dev/ttyUSB0 esp32
blusys monitor my_sensor /dev/ttyUSB0 esp32
```

## What you get

The scaffold generates a minimal headless app with:

- a `State` with a tick counter, periodic heartbeat log
- `on_tick` dispatching `Action::periodic_tick` every 100 ms
- `main/idf_component.yml` with managed git pins for all three platform tiers (plus `espressif/mdns` for connected starters); set `BLUSYS_SCAFFOLD_PLATFORM_VERSION` to change the tag
- no UI or LVGL dependencies

## Project structure

```
my_sensor/
  CMakeLists.txt       — bakes BLUSYS_BUILD_UI=OFF and project name
  sdkconfig.defaults
  main/
    CMakeLists.txt     — idf_component_register listing core + integration
    idf_component.yml
    core/
      app_logic.hpp    — State, Action, update(), on_tick() declarations
      app_logic.cpp    — reducer and tick hook implementation
    integration/
      app_main.cpp     — app_spec wiring + BLUSYS_APP_MAIN_HEADLESS(spec) macro
  host/
    CMakeLists.txt     — standalone PC build
```

## Next steps

- [Archetype Starters](archetypes.md) --- choose the canonical starting shape
- [Reducer Model](../app/reducer-model.md) --- understand state, actions, and `update()`
- [Capabilities](../app/capabilities.md) --- add WiFi, SNTP, and storage
- [Profiles](../app/profiles.md) --- understand headless and device profiles
