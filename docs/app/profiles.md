# Platform Profiles

Profiles configure the hardware and runtime environment for your app. `BLUSYS_APP(spec)` uses `spec.profile` and `spec.host_title` when you want to pin display size or window title. `BLUSYS_APP_HEADLESS(spec)` runs without UI.

## Interactive

Use `BLUSYS_APP(spec)` for host and device builds.

```cpp
BLUSYS_APP(spec)
```

To set window size and title explicitly, set `spec.profile` and `spec.host_title` before the entry macro runs.

- Simulated display window via SDL2
- Arrow keys + Enter map to encoder rotation and press
- No real hardware dependencies
- Fastest iteration cycle

### Interactive input model (host and device)

Host SDL and device encoder bridges both feed the same `blusys::intent` stream (`increment`, `decrement`, `confirm`, …). Your app handles hardware differences in reducer logic and `on_event`, not in UI code. After navigation, the framework re-attaches focus via `screen_router` so encoder and keyboard-driven runs stay consistent.

## Headless

Use `BLUSYS_APP_HEADLESS(spec)` for connected devices without a display.

```cpp
BLUSYS_APP_HEADLESS(spec)
```

- Same app runtime core as interactive
- No UI or LVGL dependencies
- `on_tick` and `on_event` for headless flows
- Builds on ESP32, ESP32-C3, and ESP32-S3

### Headless tuning

There is no separate public headless profile macro; the runtime uses its default headless profile internally.

| Surface | What to set |
|---------|-------------|
| Main loop cadence | `app_spec.tick_period_ms` — delay between `runtime.step()` calls |
| Capability events | `app_spec.on_event` — translate capability codes into product `Action`s |
| Periodic work | `app_spec.on_tick` — time-based housekeeping without UI |
| Capabilities | Register on `app_spec.capabilities` from `platform/` only |

Use the same reducer (`update`) and action model as interactive products; only the entry macro and build flags differ (`BLUSYS_BUILD_UI` off).

## Device Profile

For targeting real hardware with a display.

```cpp
#include "blusys/framework/platform/profiles/st7735.hpp"

spec.profile = &blusys::platform::st7735_160x128();
BLUSYS_APP(spec)
```

When targeting a device, set `spec.profile` to a `device_profile` factory such as `blusys::platform::st7735_160x128()` and use `BLUSYS_APP(spec)`.

### Generic SPI ST7735

The first canonical interactive hardware profile. Framework-owned display and UI bring-up for SPI TFT panels using the ST7735 controller.

- Compiles on ESP32, ESP32-C3, and ESP32-S3
- Framework owns LCD initialization, LVGL setup, and UI lock discipline
- Pin and SPI configuration through the profile struct
- Kconfig reserved for advanced tuning only

### Selecting a profile

| Entry | Profile | Display |
|-------|---------|---------|
| `BLUSYS_APP(spec)` | Host or device | Simulated or real hardware |
| `BLUSYS_APP_HEADLESS(spec)` | Headless | None |

The same `app_spec`, State, Actions, and `update()` compile for all entries. Swap the `spec.profile` assignment to switch target size.

## Stock device profile headers

Beyond ST7735, the framework ships small **data-first** profile factories under `blusys/framework/platform/profiles/`. Override fields after construction for your PCB.

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

`#include "blusys/framework/platform/layout_surface.hpp"` — `blusys::layout::classify(device_profile)` returns a small **surface_hints** struct (size class, suggested shell density, spacing/typography levels, theme packaging hint). Use it to tune shell chrome or documentation; it is **not** a responsive layout engine.

**Rotation:** Fix panel orientation with `device_profile` fields (`swap_xy`, mirrors) and HAL; `classify()` uses the **logical** width/height already mapped for the UI. When `BLUSYS_FRAMEWORK_HAS_UI` is defined, `blusys::layout::suggested_theme_density()` maps hints to a `blusys::ui::density` aligned with stock presets (`expressive_dark`, `operational_light`, `compact_dark`, `oled`).

## Kconfig in reference builds

The **blusys** component defines shared **menuconfig** choices for SPI display profiles (dashboard-class ILI9341, ILI9488, or ST7735, plus optional QEMU RGB). Individual examples add only product-specific options (e.g. Wi-Fi strings on coordinator references).

- **Headless** (`examples/validation/connected_headless`): optional **local SSD1306** status UI vs default headless (device only).

Host SDL builds for dashboard-class examples use `BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE` in `host/CMakeLists.txt` (0 = 320×240 logical, 1 = 480×320, 2 = 160×128 ST7735-class). Compact interactive host windows use `BLUSYS_IC_HOST_DISPLAY_PROFILE` for the ST7735/ST7789 host matrix.

## Interface → profile (quick reference)

| Interface | Typical profiles | Notes |
|-----------|------------------|-------|
| Interactive (compact) | ST7735, ST7789 | Compact color; expressive theme bias |
| Interactive (dashboard) | ILI9341, ILI9488 | Operational / dashboard density |
| Headless | Headless, or SSD1306 for local status | Same reducer; `platform/` chooses entry |
| Interactive (coordinator) | Headless, ILI9341, ILI9488 | Headless default; optional local operator UI |
