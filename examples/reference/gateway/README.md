# Gateway / controller (reference)

Secondary **Phase 6** connected archetype: coordinator-style product with the same capability stack as [edge node](../../quickstart/edge_node/README.md), plus an operator-oriented shell and dashboard (stock screens, `operational_light` theme).

## Layout

- `main/core/` — reducer, orchestration-oriented state (simulated downstream counts / throughput)  
- `main/ui/` — dashboard, status, settings routes  
- `main/integration/` — capabilities, event bridge, `on_tick`, platform entry  

## Build (device)

```bash
cd examples/reference/gateway
idf.py set-target esp32s3
idf.py menuconfig   # optional: Gateway display profile ILI9488
idf.py build flash monitor
```

## Build (host, SDL)

```bash
export BLUSYS_PATH="$(pwd)"
cmake -S examples/reference/gateway/host -B examples/reference/gateway/build-host
cmake --build examples/reference/gateway/build-host -j
./examples/reference/gateway/build-host/gateway_host
```

Optional: `-DBLUSYS_GW_HOST_DISPLAY_PROFILE=1` for 480×320 SDL surface.

## Docs

- [Connected products](../../../docs/start/connected-products.md)  
- [Archetype starters](../../../docs/start/archetypes.md)  
