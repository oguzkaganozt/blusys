# Platform Profiles

Profiles configure the hardware and runtime environment for your app. The framework owns display bring-up, input bridging, and runtime adapters so your app code stays the same across targets.

## Host Profile

The default for development and prototyping. Runs on your development machine with a simulated display.

```cpp
BLUSYS_APP_HOST_MAIN(spec);
```

- Simulated display window via SDL
- Keyboard and mouse input
- No real hardware dependencies
- Fastest iteration cycle

## Headless Profile

For connected devices without a display.

```cpp
BLUSYS_APP_HEADLESS_MAIN(spec);
```

- Same app runtime core as interactive
- No UI or LVGL dependencies
- Service and timer effects for headless flows
- Builds on ESP32, ESP32-C3, and ESP32-S3

## Device Profiles

For targeting real hardware with a display.

```cpp
BLUSYS_APP_MAIN(spec);
```

### Generic SPI ST7735

The first canonical interactive hardware profile. Framework-owned display and UI bring-up for SPI TFT panels using the ST7735 controller.

- Compiles on ESP32, ESP32-C3, and ESP32-S3
- Framework owns LCD initialization, LVGL setup, and UI lock discipline
- Pin and SPI configuration through code-first config
- Kconfig reserved for advanced tuning only

### Selecting a profile

Profiles are selected through the entry macro and build configuration:

| Entry Macro | Profile | Display |
|-------------|---------|---------|
| `BLUSYS_APP_HOST_MAIN` | Host (SDL) | Simulated |
| `BLUSYS_APP_MAIN` | Device | Real hardware |
| `BLUSYS_APP_HEADLESS_MAIN` | Headless | None |

The same `app_spec`, state, actions, and reducer work across all three profiles.
