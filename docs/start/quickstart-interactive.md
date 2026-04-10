# Interactive Quickstart

Build and run a display-first interactive product using the `blusys::app` path.

## Create the project

```bash
blusys create --starter interactive my_product
cd my_product
```

## Run on host

```bash
blusys host-build my_product
```

The app launches in a host simulator window with the default theme and navigation.

## What you get

The scaffold generates a minimal app with:

- an `app_spec` defining state, actions, and the reducer
- a home screen with stock widgets
- framework-owned boot, navigation, and display lifecycle
- host-first prototyping out of the box

## Project structure

```
my_product/
  main/
    app.cpp          # Your app: state, actions, update(), screens
    CMakeLists.txt
  CMakeLists.txt
  sdkconfig.defaults
```

## Next steps

- [Reducer Model](../app/reducer-model.md) --- understand state, actions, and `update()`
- [Views & Widgets](../app/views-and-widgets.md) --- build screens with stock widgets
- [Profiles](../app/profiles.md) --- target a real device with ST7735 or headless profiles
- [Service Bundles](../app/service-bundles.md) --- add WiFi, storage, and connectivity
