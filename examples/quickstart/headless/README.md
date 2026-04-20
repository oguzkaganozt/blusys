# Headless (quickstart)

Primary **headless connected** reference (`interface: headless` in `inventory.yml`): provisioning and Wi-Fi lifecycle live under **connectivity**; telemetry, diagnostics, OTA, storage, and lan_control are composed in `main/platform/` — all through `blusys::app` capabilities and a reducer-driven operational phase machine.

## Layout

- `main/core/` — state, actions, `update`, phase computation  
- `main/platform/` — capability config, event bridge, `on_tick`, entry macro  
- `main/ui/` — optional SSD1306 mono status (Kconfig); empty compile when headless-only  

## Build (device)

```bash
cd examples/quickstart/headless
idf.py set-target esp32s3   # or esp32 / esp32c3
idf.py build flash monitor
```

Set Wi-Fi via `idf.py menuconfig` → \*\*Headless example\*\* (or project `sdkconfig`).

## Build (host, headless)

From repo root:

```bash
export BLUSYS_PATH="$(pwd)"
cmake -S examples/quickstart/headless/host -B examples/quickstart/headless/build-host
cmake --build examples/quickstart/headless/build-host -j
./examples/quickstart/headless/build-host/headless_host
```

## Optional local OLED

Enable **Headless example → Local SSD1306 status OLED (128x64 I2C)** in menuconfig. Rebuild; entry becomes `BLUSYS_APP(spec)` with `ssd1306_128x64()` as the profile.

## Docs

- [Product shape](../../../docs/start/product-shape.md)
