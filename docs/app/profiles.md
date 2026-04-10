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

## Headless Profile

For connected devices without a display.

```cpp
BLUSYS_APP_MAIN_HEADLESS(spec)
```

- Same app runtime core as interactive
- No UI or LVGL dependencies
- `on_tick` and `map_event` for headless flows
- Builds on ESP32, ESP32-C3, and ESP32-S3

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

The same `app_spec`, State, Actions, and `update()` compile for all entries. Swap the macro at the bottom of `system/app_main.cpp` to switch targets.
