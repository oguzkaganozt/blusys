# Interactive dashboard (reference)

Dense LVGL dashboard: KPI row, arc gauge, rolling line trend, zone bar chart, profile
buttons, and target slider. Uses `expressive_dark`, synthetic `on_tick` metrics, and
diagnostics + storage capabilities on device.

## Host (SDL)

```bash
blusys host-build examples/reference/interactive_dashboard
```

Pick the **device display profile** (Blusys framework `menuconfig` → **Dashboard-class display (device)** on device; CMake `BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE` on host): that sets the **logical** resolution the UI is laid out for. The PC SDL window is **scaled automatically** from that resolution—you do not need a separate “host size.” Optional: `BLUSYS_HOST_ZOOM=1` disables upscaling. Profiles include ILI9341, ILI9488, ST7735 160×128, **SSD1306 128×64** (defaults: **ESP32** I2C via `sdkconfig.defaults.esp32`; **ESP32-C3** I2C **SDA=8 / SCL=9** via `sdkconfig.defaults.esp32c3` + `integration/app_main.cpp`), and QEMU virtual RGB where supported.

Run the printed path under `examples/reference/interactive_dashboard/build-host/`
(typically `interactive_dashboard_host`).

## Device

```bash
blusys build examples/reference/interactive_dashboard esp32s3
blusys flash examples/reference/interactive_dashboard /dev/ttyACM0 esp32s3
```

**ESP32-C3 + I2C SSD1306** (this repo default): `sdkconfig.defaults.esp32c3` selects SSD1306; I2C **SDA=GPIO8, SCL=GPIO9** are set in `main/integration/app_main.cpp` (`dashboard_device_profile`). For SPI ST7735 or different I2C pins, change **Dashboard-class display (device)** in `menuconfig` and adjust the profile in `integration/` (see `docs/app/profiles.md`).

```bash
blusys build examples/reference/interactive_dashboard esp32c3
blusys flash examples/reference/interactive_dashboard /dev/ttyACM0 esp32c3
```

Display profile is selectable in `menuconfig` under **Blusys framework → Display profiles → Dashboard-class display (device)**.

**Boot crash (`Stack protection fault` in task `main`):** the example sets a large **main task stack** in `sdkconfig.defaults` (16 KiB). If you still use an old `sdkconfig` from before that merge, delete `examples/reference/interactive_dashboard/sdkconfig` (or run `blusys fullclean examples/reference/interactive_dashboard esp32c3`) and rebuild so the new stack size applies. You can confirm under **Component config → ESP System Settings → Main task stack size**.

**Task watchdog on `blusys_display` / LVGL stuck in `lv_flex`:** at 160×128 the stock dashboard row layout (gauge + chart with flex grow) can stress LVGL’s flex engine. This example uses a **tiny layout** (column stack, fixed heights, shorter shell chrome without the status strip) when the display is ≤200×160 logical pixels.
