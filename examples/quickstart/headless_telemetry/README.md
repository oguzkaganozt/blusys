# Headless telemetry (quickstart)

Primary **headless connected** reference (`interface: headless` in `inventory.yml`): provisioning and Wi-Fi lifecycle live under **connectivity**; telemetry, diagnostics, OTA, storage, and lan_control are composed in `integration/` — all through `blusys::app` capabilities and a reducer-driven operational phase machine.

## Layout

- `main/core/` — state, actions, `update`, phase computation  
- `main/integration/` — capability config, event bridge, `on_tick`, entry macro  
- `main/ui/` — optional SSD1306 mono status (Kconfig); empty compile when headless-only  

## Build (device)

```bash
cd examples/quickstart/headless_telemetry
idf.py set-target esp32s3   # or esp32 / esp32c3
idf.py build flash monitor
```

Set Wi-Fi via `idf.py menuconfig` → **Headless telemetry example** (or project `sdkconfig`).

## Build (host, headless)

From repo root:

```bash
export BLUSYS_PATH="$(pwd)"
cmake -S examples/quickstart/headless_telemetry/host -B examples/quickstart/headless_telemetry/build-host
cmake --build examples/quickstart/headless_telemetry/build-host -j
./examples/quickstart/headless_telemetry/build-host/headless_telemetry_host
```

## Optional local OLED

Enable **Headless telemetry example → Local SSD1306 status OLED (128x64 I2C)** in menuconfig. Rebuild; entry becomes `BLUSYS_APP_MAIN_DEVICE` with `ssd1306_128x64` profile.

## Docs

- [Product shape](../../../docs/start/product-shape.md)
