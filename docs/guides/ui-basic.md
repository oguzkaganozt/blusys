# Render a UI with LVGL

## Problem Statement

You want to display widgets (labels, buttons, gauges) on an LCD panel using LVGL. The UI runs in a dedicated FreeRTOS task; your application code creates and updates widgets from other tasks.

## Prerequisites

- An LCD panel opened with `blusys_lcd_open()` — the UI service wraps it and takes ownership of flushing.
- The `espressif/lvgl` managed component declared in your project's `idf_component.yml`. The `blusys_services` CMakeLists.txt detects it and defines `BLUSYS_HAS_LVGL` when present.
- The `blusys_ui_t` is a **singleton** — only one instance can be open at a time.

## Minimal Example

```c
#include "blusys/blusys_all.h"
#include "lvgl.h"

void app_main(void)
{
    /* 1. Open the LCD */
    blusys_lcd_t *lcd = NULL;
    blusys_lcd_config_t lcd_cfg = {
        .driver         = BLUSYS_LCD_DRIVER_ST7735,
        .width          = 160,
        .height         = 128,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .swap_xy        = true,
        .mirror_x       = true,
        .mirror_y       = false,
        .invert_color   = false,
        .spi = {
            .bus      = 0,
            .sclk_pin = 18,
            .mosi_pin = 23,
            .cs_pin   = 5,
            .dc_pin   = 16,
            .rst_pin  = 17,
            .bl_pin   = 4,
            .pclk_hz  = 20000000,
        },
    };
    blusys_lcd_open(&lcd_cfg, &lcd);

    /* 2. Open the UI (starts LVGL render task) */
    blusys_ui_t *ui = NULL;
    blusys_ui_config_t ui_cfg = {
        .lcd = lcd,
        .full_refresh = false,
    };
    blusys_ui_open(&ui_cfg, &ui);

    /* 3. Create widgets — always lock before touching LVGL state */
    blusys_ui_lock(ui);

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello");
    lv_obj_center(label);

    blusys_ui_unlock(ui);

    /* 4. Cleanup */
    blusys_ui_close(ui);
    blusys_lcd_close(lcd);
}
```

For common 128x160 ST7735 portrait modules, `swap_xy = true` and `mirror_x = true` is a good starting point. When `swap_xy = true`, set `width` and `height` to the logical rotated size (`160x128` here). If text is mirrored or black/white are inverted on your module, adjust `mirror_x`, `mirror_y`, or `invert_color`.

For small SPI TFTs such as 128x160 ST7735 panels, keep `full_refresh = false` unless you have hardware-validated the full-screen path on your module. The default partial mode is still the only hardware-validated safe path at the moment. It flushes LVGL output conservatively row-by-row, which is slower on full-screen redraws but avoids the corruption seen with the current accelerated path experiments.

## APIs Used

| Function | Purpose |
| --- | --- |
| `blusys_ui_open()` | Initialise LVGL, create draw buffers, start render task |
| `blusys_ui_close()` | Stop render task, free LVGL resources |
| `blusys_ui_lock()` | Acquire LVGL mutex before widget access |
| `blusys_ui_unlock()` | Release LVGL mutex |

## Expected Behavior

After `blusys_ui_open()` returns, a render task runs in the background calling `lv_timer_handler()` continuously. Any widgets created between `blusys_ui_lock()` / `blusys_ui_unlock()` appear on screen within one render cycle (≤ 500 ms).

## Common Mistakes

**Touching LVGL without the lock.** The render task calls LVGL from its own context. Calling any `lv_*` function from another task without holding the lock causes data corruption. Always bracket widget code with `blusys_ui_lock()` / `blusys_ui_unlock()`.

**Closing the LCD before the UI.** `blusys_ui_close()` must be called before `blusys_lcd_close()` — the render task flushes to the LCD right up until it exits.

**Opening more than one UI instance.** `blusys_ui_open()` returns `BLUSYS_ERR_INVALID_STATE` if called while a UI is already open. Close the existing instance first.

## Example App

See `examples/ui_basic/` for a complete project that opens a ST7735 LCD, initialises LVGL, and renders a centered label.

## API Reference

[UI Module →](../modules/ui.md)
