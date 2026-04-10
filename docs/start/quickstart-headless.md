# Headless Quickstart

Build a connected device without a display using the same `blusys::app` reducer model.

## Create the project

```bash
blusys create --starter headless my_sensor
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
- no UI or LVGL dependencies

## Project structure

```
my_sensor/
  CMakeLists.txt       — bakes BLUSYS_BUILD_UI=OFF and project name
  sdkconfig.defaults
  main/
    CMakeLists.txt     — idf_component_register listing logic + system
    idf_component.yml
    logic/
      app_logic.hpp    — State, Action, update(), on_tick() declarations
      app_logic.cpp    — reducer and tick hook implementation
    system/
      app_main.cpp     — app_spec wiring + BLUSYS_APP_MAIN_HEADLESS(spec) macro
  host/
    CMakeLists.txt     — standalone PC build
```

## Next steps

- [Reducer Model](../app/reducer-model.md) --- understand state, actions, and `update()`
- [Capabilities](../app/capabilities.md) --- add WiFi, SNTP, and storage
- [Profiles](../app/profiles.md) --- understand headless and device profiles
