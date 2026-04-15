# Interactive Quickstart

Build and run a display-first interactive product using the `blusys::app` path.

The default interactive starting point uses the **`handheld`** interface (compact shell, tactile bias):

- example: `examples/quickstart/handheld/`
- visual bias: expressive and tactile
- interaction bias: compact, encoder-friendly control flows
- architecture: the same reducer, shell, and capability model used everywhere else

## Create the project

```bash
blusys create my_product
# equivalent: blusys create --interface handheld my_product
cd my_product
```

For a denser operator shell, use **`--interface surface`** (see [Product shape](product-shape.md)).

## Run on host

```bash
blusys host-build my_product
```

The app launches in a host SDL2 window with the default theme. Arrow keys move focus between buttons; Enter activates. No hardware needed.

## What you get

The scaffold generates a host-first interactive app with:

- framework-owned boot, LVGL lifecycle, shell, and focus wiring
- the canonical `core/`, `ui/`, and `platform/` product structure (CI-enforced for product-shaped examples via `scripts/check-product-layout.py`)
- `main/idf_component.yml` with managed git pins for the platform component (and LVGL); set `BLUSYS_SCAFFOLD_PLATFORM_VERSION` when you need a different release tag
- a reducer-owned control flow that can be retargeted to host or ST7735 device profiles

If you want a concrete product-shaped reference immediately, start from:

- `examples/quickstart/handheld/` for compact expressive control surfaces (see its `README.md`)
- `examples/reference/display/` for display / LVGL / encoder scenarios (menuconfig)

## Project structure

```
my_product/
  CMakeLists.txt       — bakes BLUSYS_BUILD_UI=ON and project name
  sdkconfig.defaults
  blusys.project.yml   — declared interface, capabilities, policies
  main/
    CMakeLists.txt     — idf_component_register listing core, ui, and platform
    idf_component.yml
    core/
      app_logic.hpp    — State, Action, update(); domain-only (no LVGL component defs)
      app_logic.cpp    — reducer implementation
    ui/
      panels.hpp       — product panel types + sync from state (custom “components”)
      panels.cpp
      app_ui.cpp       — screen factories, shell chrome wiring
    platform/
      app_main.cpp     — app_spec wiring + BLUSYS_APP_MAIN_HOST(spec) macro
  host/
    CMakeLists.txt     — standalone PC build (cmake -S host -B build-host)
```

## Next steps

- [Product shape](product-shape.md) — choose interface, capabilities, and policies
- [Reducer Model](../app/reducer-model.md) — understand state, actions, and `update()`
- [Views & Widgets](../app/views-and-widgets.md) — build screens with stock widgets
- [Profiles](../app/profiles.md) — target a real device with `BLUSYS_APP_MAIN_DEVICE`
- [Capabilities](../app/capabilities.md) — add WiFi and storage
