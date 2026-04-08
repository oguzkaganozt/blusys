# UI

LVGL-based UI service. Opens an LCD handle, initialises LVGL, allocates draw buffers, and runs a dedicated render task. Widget access from other tasks is serialised through `blusys_ui_lock()` / `blusys_ui_unlock()`.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Render a UI with LVGL](../guides/ui-basic.md).

## Target Support

| Target | Supported |
| ------ | --------- |
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note "Requires LVGL"
    This module compiles only when the `espressif/lvgl` managed component is present in the project. Declare it in your `idf_component.yml`. Without it, all functions return `BLUSYS_ERR_NOT_SUPPORTED`.

## Singleton

Only one UI instance may be open at a time. `blusys_ui_open()` returns `BLUSYS_ERR_INVALID_STATE` if called while an instance already exists.

## Types

```c
typedef struct blusys_ui blusys_ui_t;
```

```c
typedef struct {
    blusys_lcd_t *lcd;           /* Required: already-opened LCD handle */
    uint32_t      buf_lines;     /* Draw buffer height in lines (0 = default 20) */
    bool          full_refresh;  /* Use one full-screen render buffer (experimental on SPI LCDs) */
    int           task_priority; /* LVGL render task priority (0 = default 5) */
    int           task_stack;    /* Render task stack in bytes (0 = default 16384) */
} blusys_ui_config_t;
```

## Functions

### `blusys_ui_open`

```c
blusys_err_t blusys_ui_open(const blusys_ui_config_t *config,
                             blusys_ui_t **out_ui);
```

Initialises LVGL, allocates draw buffers, registers the LCD flush callback, and starts the render task. By default the UI uses two partial buffers of `buf_lines` rows each, copies each LVGL flush area into a DMA-safe scratch buffer, and pushes it to the LCD in packed multi-row bands. When `full_refresh = true`, it allocates one full-screen render buffer but still flushes through the same scratch-buffer band path. That full-screen SPI path is still experimental.

| Parameter | Description |
| --- | --- |
| `config` | Configuration; `config->lcd` must be a valid open handle |
| `out_ui` | Receives the UI handle on success |

Returns `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE` (already open), or `BLUSYS_ERR_NO_MEM`.

### `blusys_ui_close`

```c
blusys_err_t blusys_ui_close(blusys_ui_t *ui);
```

Signals the render task to stop, waits for it to exit, then frees LVGL resources and draw buffers. Call before `blusys_lcd_close()`.

### `blusys_ui_lock`

```c
blusys_err_t blusys_ui_lock(blusys_ui_t *ui);
```

Acquires the LVGL global mutex. Must be called before any `lv_*` API from a task other than the render task. Blocks until the mutex is available.

### `blusys_ui_unlock`

```c
blusys_err_t blusys_ui_unlock(blusys_ui_t *ui);
```

Releases the LVGL global mutex. Always pair with `blusys_ui_lock()`.

## Lifecycle

```text
blusys_lcd_open()   →   blusys_ui_open()
                              ↓
                    [render task running]
                              ↓
                    blusys_ui_close()   →   blusys_lcd_close()
```

## Thread Safety

The render task calls `lv_timer_handler()` in a loop. All other tasks must bracket LVGL calls with `blusys_ui_lock()` / `blusys_ui_unlock()`. The `blusys_ui_open()` and `blusys_ui_close()` functions are not themselves thread-safe — call them from a single task (typically `app_main`).

## Limitations

- Only one UI instance at a time (singleton).
- Requires `espressif/lvgl` managed component.
- The flush callback byte-swaps RGB565 pixels in its temporary scratch buffer for SPI LCD compatibility.
- Draw buffer size is `buf_lines × stride` bytes, allocated twice (double buffering). For a 320-pixel-wide display at 20 lines, each buffer is 320 × 20 × 2 = 12 800 bytes (25 600 bytes total).
- When `full_refresh` is enabled, `buf_lines` is ignored and the UI allocates one screen-sized render buffer plus the same temporary flush band buffer. This uses more RAM and should be treated as experimental on SPI LCDs until validated on your panel.

## Example App

`examples/ui_basic/` — opens an ST7735 LCD and renders a centered label.
