# Interactive Quickstart

Build a **minimal `interactive` app** (SDL host window, optional `main/ui/`). The only scaffold difference from [Headless quickstart](quickstart-headless.md) is `--interface interactive` and the `ui/` folder.

!!! tip
    Manifest vocabulary (`schema`, `interface`, `capabilities`, …) is in [Product shape](product-shape.md). This page is the shortest path to a running binary.

## Create the project

**Interactive** = `--interface interactive` and a `main/ui/` directory (empty until you add screens).

```bash
blusys create --interface interactive my_product
cd my_product
```

??? note "Generated layout"
    The starter is intentionally thin:

    ```text
    my_product/
      blusys.project.yml
      main/
        app_main.cpp
        ui/
    ```

`main/app_main.cpp` holds `State`, `Action`, and `update(ctx, state, action)`; add screens under `main/ui/` when needed.

## Run on host

```bash
blusys host-build
```

The app opens an SDL2 window with the default theme. Arrow keys move focus; Enter activates. No hardware required.

## When to expand

- Add **`capabilities`** in the manifest for connectivity, storage, telemetry, OTA, or other services.
- Add **`profile`** for a specific host or device setup.
- Split `app_main.cpp` when it stops being readable.

## Next steps

- [Product shape](product-shape.md) — interface, capabilities, profiles, policies
- [Reducer model](../app/reducer-model.md) — state, actions, `update()`
- [Views & widgets](../app/views-and-widgets.md) — stock widgets
- [Profiles](../app/profiles.md) — device vs host
- [Capabilities](../app/capabilities.md) — WiFi, storage, and more
