# UI

!!! note "Internal service"
    This is a low-level service used internally by the framework. Product code should use [`blusys::app` profiles](../app/profiles.md) and the [view layer](../app/views-and-widgets.md) instead of calling these APIs directly.

LVGL-based UI service. Opens an LCD handle, initialises LVGL, allocates draw buffers, and runs a dedicated render task. Widget access from other tasks is serialised through `blusys_display_lock()` / `blusys_display_unlock()`.

## Target Support

| Target | Supported |
| ------ | --------- |
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note "Requires LVGL"
    This module compiles only when the `lvgl/lvgl` managed component is present in the project. Declare it in your `idf_component.yml`. Without it, all functions return `BLUSYS_ERR_NOT_SUPPORTED`.

## Singleton

Only one UI instance may be open at a time. `blusys_display_open()` returns `BLUSYS_ERR_INVALID_STATE` if called while an instance already exists.

## Quick Example

```c
#include "blusys/blusys.h"
#include "blusys/blusys.h"
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
    blusys_display_t *ui = NULL;
    blusys_display_config_t ui_cfg = {
        .lcd = lcd,
        .full_refresh = false,
    };
    blusys_display_open(&ui_cfg, &ui);

    /* 3. Create widgets — always lock before touching LVGL state */
    blusys_display_lock(ui);

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello");
    lv_obj_center(label);

    blusys_display_unlock(ui);

    /* 4. Cleanup */
    blusys_display_close(ui);
    blusys_lcd_close(lcd);
}
```

For common 128x160 ST7735 portrait modules, `swap_xy = true` and `mirror_x = true` is a good starting point. When `swap_xy = true`, set `width` and `height` to the logical rotated size (`160x128` here). If text is mirrored or black/white are inverted on your module, adjust `mirror_x`, `mirror_y`, or `invert_color`.

For small SPI TFTs such as 128x160 ST7735 panels, keep `full_refresh = false` unless you have hardware-validated the full-screen path on your module. The default partial mode is the recommended path: it copies each LVGL flush area into a DMA-safe scratch buffer and sends it to the LCD in packed multi-row bands, which avoids the corruption seen when DMAing directly from LVGL-owned buffers.

## Common Mistakes

**Touching LVGL without the lock.** The render task calls LVGL from its own context. Calling any `lv_*` function from another task without holding the lock causes data corruption. Always bracket widget code with `blusys_display_lock()` / `blusys_display_unlock()`.

**Closing the LCD before the UI.** `blusys_display_close()` must be called before `blusys_lcd_close()` — the render task flushes to the LCD right up until it exits.

**Opening more than one UI instance.** `blusys_display_open()` returns `BLUSYS_ERR_INVALID_STATE` if called while a UI is already open. Close the existing instance first.

## Types

```c
typedef struct blusys_display blusys_display_t;
```

```c
typedef enum {
    BLUSYS_DISPLAY_PANEL_KIND_RGB565         = 0, /* Default: byte-swap RGB565 for 16-bit SPI panels (ST7735, ILI9341, …) */
    BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE      = 1, /* Threshold RGB565 → 1-bpp SSD1306-style page buffer (forces full refresh) */
    BLUSYS_DISPLAY_PANEL_KIND_RGB565_NATIVE  = 2, /* Native LVGL RGB565 order, no byte swap (e.g. esp_lcd_qemu_rgb) */
} blusys_display_panel_kind_t;

typedef struct {
    blusys_lcd_t           *lcd;           /* Required: already-opened LCD handle */
    uint32_t                buf_lines;     /* Draw buffer height in lines (0 = default 20) */
    bool                    full_refresh;  /* Use one full-screen render buffer (experimental on SPI LCDs) */
    int                     task_priority; /* LVGL render task priority (0 = default 5) */
    int                     task_stack;    /* Render task stack in bytes (0 = default 16384) */
    blusys_display_panel_kind_t  panel_kind;    /* Pixel-format family (default 0 = RGB565) */
} blusys_display_config_t;
```

## Functions

### `blusys_display_open`

```c
blusys_err_t blusys_display_open(const blusys_display_config_t *config,
                             blusys_display_t **out_ui);
```

Initialises LVGL, allocates draw buffers, registers the LCD flush callback, and starts the render task. By default the UI uses two partial buffers of `buf_lines` rows each, copies each LVGL flush area into a DMA-safe scratch buffer, and pushes it to the LCD in packed multi-row bands. When `full_refresh = true`, it allocates one full-screen render buffer but still flushes through the same scratch-buffer band path. That full-screen SPI path is still experimental.

| Parameter | Description |
| --- | --- |
| `config` | Configuration; `config->lcd` must be a valid open handle |
| `out_ui` | Receives the UI handle on success |

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE` (already open), or `BLUSYS_ERR_NO_MEM`.

### `blusys_display_close`

```c
blusys_err_t blusys_display_close(blusys_display_t *ui);
```

Signals the render task to stop, waits for it to exit, then frees LVGL resources and draw buffers. Call before `blusys_lcd_close()`.

### `blusys_display_lock`

```c
blusys_err_t blusys_display_lock(blusys_display_t *ui);
```

Acquires the LVGL global mutex. Must be called before any `lv_*` API from a task other than the render task. Blocks until the mutex is available.

### `blusys_display_unlock`

```c
blusys_err_t blusys_display_unlock(blusys_display_t *ui);
```

Releases the LVGL global mutex. Always pair with `blusys_display_lock()`.

### `blusys_display_invalidation_begin` / `blusys_display_invalidation_end`

```c
void blusys_display_invalidation_begin(blusys_display_t *ui);
void blusys_display_invalidation_end(blusys_display_t *ui);
```

Batch UI construction when building deep widget trees under the UI lock. LVGL 9 must not queue invalidations while the display is mid-refresh; bracketing theme + first screen assembly with these calls (while holding the lock) prevents assertions and wedged main tasks on slow MCUs. `end()` re-enables invalidation and marks the active screen dirty.

## Lifecycle

```text
blusys_lcd_open()   →   blusys_display_open()
                              ↓
                    [render task running]
                              ↓
                    blusys_display_close()   →   blusys_lcd_close()
```

## Thread Safety

The render task calls `lv_timer_handler()` in a loop. All other tasks must bracket LVGL calls with `blusys_display_lock()` / `blusys_display_unlock()`. The `blusys_display_open()` and `blusys_display_close()` functions are not themselves thread-safe — call them from a single task (typically `app_main`).

## Limitations

- Only one UI instance at a time (singleton).
- Requires `lvgl/lvgl` managed component.
- The flush callback byte-swaps RGB565 pixels in its temporary scratch buffer for SPI LCD compatibility.
- Draw buffer size is `buf_lines × stride` bytes, allocated twice (double buffering). For a 320-pixel-wide display at 20 lines, each buffer is 320 × 20 × 2 = 12 800 bytes (25 600 bytes total).
- When `full_refresh` is enabled, `buf_lines` is ignored and the UI allocates one screen-sized render buffer plus the same temporary flush band buffer. This uses more RAM and should be treated as experimental on SPI LCDs until validated on your panel.

## Example App

`examples/reference/ui_basic/` — opens an ST7735 LCD and renders a centered label.
