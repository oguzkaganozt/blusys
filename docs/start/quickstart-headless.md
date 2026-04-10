# Headless Quickstart

Build a connected device without a display using the same `blusys::app` reducer model.

## Create the project

```bash
blusys create --starter headless my_sensor
cd my_sensor
```

## Build for a target

```bash
blusys build my_sensor esp32
blusys flash my_sensor /dev/ttyUSB0 esp32
blusys monitor my_sensor /dev/ttyUSB0 esp32
```

## What you get

The scaffold generates a minimal headless app with:

- an `app_spec` defining state, actions, and the reducer
- service and timer effects for headless flows
- connectivity and storage bundles ready to enable
- no UI or LVGL dependencies

## Project structure

```
my_sensor/
  main/
    app.cpp          # Your app: state, actions, update(), effects
    CMakeLists.txt
  CMakeLists.txt
  sdkconfig.defaults
```

## Next steps

- [Reducer Model](../app/reducer-model.md) --- understand state, actions, and `update()`
- [Service Bundles](../app/service-bundles.md) --- add WiFi, MQTT, storage, and SNTP
- [Profiles](../app/profiles.md) --- understand headless and device profiles
