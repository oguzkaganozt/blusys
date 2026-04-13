# Surface gateway (reference)

Secondary **connected reference** (`interface: surface` in `inventory.yml`, coordinator-style): same capability stack as [headless telemetry](../../quickstart/headless_telemetry/README.md), plus an operator-oriented shell and dashboard (stock screens, `operational_light` theme).

## Layout

- `main/core/` — reducer, orchestration-oriented state (simulated downstream counts / throughput)  
- `main/ui/` — dashboard, status, settings routes  
- `main/integration/` — capabilities, event bridge, `on_tick`, platform entry  

## Build (device)

```bash
cd examples/reference/surface_gateway
idf.py set-target esp32s3
idf.py menuconfig   # optional: surface / ILI9488 display profile
idf.py build flash monitor
```

## Build (host, SDL)

```bash
export BLUSYS_PATH="$(pwd)"
cmake -S examples/reference/surface_gateway/host -B examples/reference/surface_gateway/build-host
cmake --build examples/reference/surface_gateway/build-host -j
./examples/reference/surface_gateway/build-host/surface_gateway_host
```

Optional: `-DBLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE=1` for 480×320 SDL surface.

## Docs

- [Product shape](../../../docs/start/product-shape.md)
