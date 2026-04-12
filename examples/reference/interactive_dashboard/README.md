# Interactive dashboard (reference)

Dense LVGL dashboard: KPI row, arc gauge, rolling line trend, zone bar chart, profile
buttons, and target slider. Uses `expressive_dark`, synthetic `on_tick` metrics, and
diagnostics + storage capabilities on device.

## Host (SDL)

```bash
blusys host-build examples/reference/interactive_dashboard
```

Pick the **device display profile** (Blusys framework `menuconfig` on device; CMake `BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE` on host): that sets the **logical** resolution the UI is laid out for. The PC SDL window is **scaled automatically** from that resolution—you do not need a separate “host size.” Optional: `BLUSYS_HOST_ZOOM=1` disables upscaling.

Run the printed path under `examples/reference/interactive_dashboard/build-host/`
(typically `interactive_dashboard_host`).

## Device

```bash
blusys build examples/reference/interactive_dashboard esp32s3
blusys flash examples/reference/interactive_dashboard /dev/ttyACM0 esp32s3
```

Display profile is selectable in `menuconfig` under **Interactive Dashboard (display)**.
