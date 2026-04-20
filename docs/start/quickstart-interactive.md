# Interactive Quickstart

Build a minimal interactive product using the manifest-first starter.

## Create the project

```bash
blusys create --interface interactive my_product
cd my_product
```

The starter keeps the first scaffold intentionally thin:

```text
my_product/
  blusys.project.yml
  main/
    app_main.cpp
    ui/
```

`blusys.project.yml` declares the product shape: `schema`, `interface`, `capabilities`, `profile`, and `policies`. `main/app_main.cpp` owns `State`, `Action`, `update(ctx, state, action)`, and app wiring. `main/ui/` stays empty until the product actually needs screens or widgets.

## Run on host

```bash
blusys host-build my_product
```

The app launches in a host SDL2 window with the default theme. Arrow keys move focus between buttons; Enter activates. No hardware needed.

## When to expand

- Add `capabilities` in the manifest when the app needs connectivity, storage, telemetry, OTA, or other runtime services.
- Add `flows` when the product has distinct UI journeys.
- Add `profile` when you need a specific host or device target setup.
- Split code out of `app_main.cpp` only when the file stops being readable.

## Next steps

- [Product shape](product-shape.md) — choose interface, capabilities, flows, profiles, and policies
- [Reducer Model](../app/reducer-model.md) — understand state, actions, and `update()`
- [Views & Widgets](../app/views-and-widgets.md) — build screens with stock widgets
- [Profiles](../app/profiles.md) — target a real device with `BLUSYS_APP` and a device profile
- [Capabilities](../app/capabilities.md) — add WiFi and storage
