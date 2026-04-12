# Platform Profiles

Profiles configure the hardware and runtime environment for your app. The framework owns display bring-up, input bridging, and runtime adapters so your app code stays the same across targets.

## Host Profile

The default for development and prototyping. Runs on your development machine with a simulated display.

```cpp
BLUSYS_APP_MAIN_HOST(spec)
```

To set window size and title explicitly, use `host_profile`:

```cpp
#include "blusys/app/entry.hpp"

BLUSYS_APP_MAIN_HOST_PROFILE(spec, blusys::app::host_profile{
    .hor_res = 320,
    .ver_res = 240,
    .title   = "My Product",
})
```

- Simulated display window via SDL2
- Arrow keys + Enter map to encoder rotation and press
- No real hardware dependencies
- Fastest iteration cycle

### Interactive input model (host and device)

Host SDL and device encoder bridges both feed the **same** `blusys::framework::intent` stream (`increment`, `decrement`, `confirm`, …). Your app handles hardware differences only in `app_spec.map_intent`, not in UI code. After navigation, the framework re-attaches focus via `screen_router` so encoder and keyboard-driven runs stay consistent.

## Headless Profile

For connected devices without a display.

```cpp
BLUSYS_APP_MAIN_HEADLESS(spec)
```

Optional explicit profile (same pattern as `host_profile`):

```cpp
#include "blusys/app/profiles/headless.hpp"

BLUSYS_APP_MAIN_HEADLESS_PROFILE(spec, blusys::app::profiles::headless_profile{
    .boot_log_label = "my-sensor",
})
```

| Field | Role |
|-------|------|
| `boot_log_label` | Optional string for the first post-init log line (nullptr = default message only) |

- Same app runtime core as interactive
- No UI or LVGL dependencies
- `on_tick` and `map_event` for headless flows
- Builds on ESP32, ESP32-C3, and ESP32-S3

### Headless tuning (code-first)

There is no separate “headless hardware” struct: behavior is configured on `app_spec` and in `integration/`:

| Surface | What to set |
|---------|-------------|
| Main loop cadence | `app_spec.tick_period_ms` — delay between `runtime.step()` calls |
| Capability events | `app_spec.map_event` — translate capability codes into product `Action`s |
| Periodic work | `app_spec.on_tick` — time-based housekeeping without UI |
| Capabilities | Register on `app_spec.capabilities` from `integration/` only |

Use the same reducer (`update`) and action model as interactive products; only the entry macro and build flags differ (`BLUSYS_BUILD_UI` off).

## Device Profile

For targeting real hardware with a display.

```cpp
#include "blusys/app/profiles/st7735.hpp"

BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())
```

### Generic SPI ST7735

The first canonical interactive hardware profile. Framework-owned display and UI bring-up for SPI TFT panels using the ST7735 controller.

- Compiles on ESP32, ESP32-C3, and ESP32-S3
- Framework owns LCD initialization, LVGL setup, and UI lock discipline
- Pin and SPI configuration through the profile struct
- Kconfig reserved for advanced tuning only

### Selecting a profile

| Entry Macro | Profile | Display |
|-------------|---------|---------|
| `BLUSYS_APP_MAIN_HOST(spec)` | Host (SDL2) | Simulated |
| `BLUSYS_APP_MAIN_HOST_PROFILE(spec, p)` | Host with explicit size | Simulated |
| `BLUSYS_APP_MAIN_DEVICE(spec, profile)` | Device | Real hardware |
| `BLUSYS_APP_MAIN_HEADLESS(spec)` | Headless | None |
| `BLUSYS_APP_MAIN_HEADLESS_PROFILE(spec, profile)` | Headless + `headless_profile` | None |

The same `app_spec`, State, Actions, and `update()` compile for all entries. Swap the macro at the bottom of `integration/app_main.cpp` to switch targets.

## Stock device profile headers

Beyond ST7735, the framework ships small **data-first** profile factories under `blusys/app/profiles/`. Override fields after construction for your PCB.

| Header | Factory (typical) | Role |
|--------|-------------------|------|
| `st7735.hpp` | `st7735_160x128()` | Canonical compact SPI TFT |
| `st7789.hpp` | `st7789_320x240()` | Larger handheld / compact dashboard |
| `ssd1306.hpp` | `ssd1306_128x64()`, `ssd1306_128x32()` | I2C mono local status |
| `ili9341.hpp` | `ili9341_320x240()` | Medium operator / panel |
| `ili9488.hpp` | `ili9488_480x320()` | Large local surface |

SPI defaults use target-gated pins where common; I2C defaults follow the `lcd_ssd1306_basic` example per target.

### SH1106

Many small OLED modules use an SSD1306-compatible controller; some use **SH1106** (similar I2C wiring, different memory layout). The framework ships **SSD1306** as the mono OLED profile. Add a dedicated SH1106 HAL + profile only when product hardware cannot use the SSD1306 driver path.

## Layout hints

`#include "blusys/app/layout_surface.hpp"` — `blusys::app::layout::classify(device_profile)` returns a small **surface_hints** struct (size class, suggested shell density, spacing/typography levels, theme packaging hint). Use it to tune shell chrome or documentation; it is **not** a responsive layout engine.

**Rotation:** Fix panel orientation with `device_profile` fields (`swap_xy`, mirrors) and HAL; `classify()` uses the **logical** width/height already mapped for the UI. When `BLUSYS_FRAMEWORK_HAS_UI` is defined, `blusys::app::layout::suggested_theme_density()` maps hints to a `blusys::ui::density` aligned with stock presets (`expressive_dark`, `operational_light`, `oled`).

## Kconfig in reference examples

The **blusys_framework** component defines shared **menuconfig** choices for SPI display profiles (dashboard-class ILI9341 vs ILI9488; interactive-controller ST7735 vs ST7789). Individual examples add only product-specific options (e.g. WiFi strings on gateway).

- **Edge node** (`quickstart/edge_node`): optional **local SSD1306** status UI vs default headless (device only) — still in the example’s `main/Kconfig.projbuild`.

Host SDL builds for dashboard-class examples use `BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE` in `host/CMakeLists.txt` (0 = 320×240 logical, 1 = 480×320). The interactive controller quickstart uses `BLUSYS_IC_HOST_DISPLAY_PROFILE` for its ST7735/ST7789 host window matrix.

## Archetype → profile (quick reference)

| Archetype | Typical profiles | Notes |
|-----------|------------------|--------|
| Interactive controller | ST7735, ST7789 | Compact color; expressive theme bias |
| Interactive panel | ILI9341, ILI9488 | Operational / dashboard density |
| Edge node | Headless, or SSD1306 for local status | Same reducer; `integration/` chooses entry |
| Gateway/controller | Headless, ILI9341, ILI9488 | Headless default; optional local operator UI |
