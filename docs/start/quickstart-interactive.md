# Interactive Quickstart

Build and run a display-first interactive product using the `blusys::app` path.

The canonical interactive starting point is the `interactive controller` archetype:

- example: `examples/quickstart/interactive_controller/`
- visual bias: expressive and tactile
- interaction bias: compact, encoder-friendly control flows
- architecture: the same reducer, shell, and capability model used everywhere else

## Create the project

```bash
blusys create --archetype interactive-controller my_product
cd my_product
```

## Run on host

```bash
blusys host-build my_product
```

The app launches in a host SDL2 window with the default theme. Arrow keys move focus between buttons; Enter activates. No hardware needed.

## What you get

The scaffold generates a host-first interactive app with:

- framework-owned boot, LVGL lifecycle, shell, and focus wiring
- the canonical `core/`, `ui/`, and `integration/` product structure
- a reducer-owned control flow that can be retargeted to host or ST7735 device profiles

If you want a concrete product-shaped reference immediately, start from:

- `examples/quickstart/interactive_controller/` for compact expressive control surfaces
- `examples/reference/interactive_panel/` for denser operational panels

## Project structure

```
my_product/
  CMakeLists.txt       — bakes BLUSYS_BUILD_UI=ON and project name
  sdkconfig.defaults
  main/
    CMakeLists.txt     — idf_component_register listing core, ui, and integration
    idf_component.yml
    core/
      app.hpp          — State, Action, update() declarations
      app.cpp          — reducer implementation
    ui/
      app_ui.cpp       — screen composition with stock widgets
    integration/
      app_main.cpp     — app_spec wiring + BLUSYS_APP_MAIN_HOST(spec) macro
  host/
    CMakeLists.txt     — standalone PC build (cmake -S host -B build-host)
```

## Next steps

- [Archetype Starters](archetypes.md) --- choose between the four canonical starting shapes
- [Reducer Model](../app/reducer-model.md) --- understand state, actions, and `update()`
- [Views & Widgets](../app/views-and-widgets.md) --- build screens with stock widgets
- [Profiles](../app/profiles.md) --- target a real device with `BLUSYS_APP_MAIN_DEVICE`
- [Capabilities](../app/capabilities.md) --- add WiFi and storage
