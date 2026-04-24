# Profiles

Profiles describe the hardware preset the framework uses to bring up display and input. They change geometry and mapping; they do not change reducer logic.

## Choose the entry point

| Use this | When |
|----------|------|
| `BLUSYS_APP(spec)` | interactive product with a local UI |
| `BLUSYS_APP_HEADLESS(spec)` | no local UI |
| `spec.profile = &blusys::platform::st7735_160x128()` | the canonical compact SPI TFT |

## What a profile changes

- display size and panel kind
- rotation and mirror settings
- encoder, button, or touch mapping
- initial brightness
- host window defaults for interactive runs

## Canonical profiles

| Profile | Best for | Interface |
|---------|----------|-----------|
| `st7735_160x128` | compact color panel | interactive |
| `st7789_320x240` | larger dashboard panel | interactive |
| `ili9341_320x240` | medium panel | interactive |
| `ili9488_480x320` | large panel | interactive |
| `ssd1306_128x64` / `ssd1306_128x32` | mono status display | interactive |
| `qemu_rgb_dashboard_320x240` | virtual framebuffer | interactive |

## Example

```cpp
#include "blusys/framework/platform/profiles/st7735.hpp"

spec.profile = &blusys::platform::st7735_160x128();
BLUSYS_APP(spec)
```

```cpp
BLUSYS_APP_HEADLESS(spec)
```

Headless products do not select a display profile.

## What it does not change

- the reducer model
- the capability list
- product state or actions

## Next steps

- [Product shape](../start/product-shape.md)
- [App](index.md)
- [Views & widgets](views-and-widgets.md)
- [Device, host & QEMU CLI](cli-host-qemu.md)
