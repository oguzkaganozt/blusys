# Edge node (quickstart)

Primary **connected archetype** reference: headless-first connected product with provisioning, connectivity, telemetry, diagnostics, OTA, and storage — all through `blusys::app` capabilities and a reducer-driven operational phase machine.

## Layout

- `main/core/` — state, actions, `update`, phase computation  
- `main/integration/` — capability config, event bridge, `on_tick`, entry macro  
- `main/ui/` — optional SSD1306 mono status (Kconfig); empty compile when headless-only  

## Build (device)

```bash
cd examples/quickstart/edge_node
idf.py set-target esp32s3   # or esp32 / esp32c3
idf.py build flash monitor
```

Set Wi-Fi via `idf.py menuconfig` → **Edge Node Example** (or project `sdkconfig`).

## Build (host, headless)

From repo root:

```bash
export BLUSYS_PATH="$(pwd)"
cmake -S examples/quickstart/edge_node/host -B examples/quickstart/edge_node/build-host
cmake --build examples/quickstart/edge_node/build-host -j
./examples/quickstart/edge_node/build-host/edge_node_host
```

## Optional local OLED

Enable **Edge Node Example → Local SSD1306 status OLED** in menuconfig. Rebuild; entry becomes `BLUSYS_APP_MAIN_DEVICE` with `ssd1306_128x64` profile.

## Docs

- [Archetype Starters — connected archetypes](../../../docs/start/archetypes.md#connected-archetypes)
