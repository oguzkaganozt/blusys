# Blusys UI Overview

## Relevant Paths

- `components/blusys_services/src/display/lcd.c`
- `components/blusys_services/src/display/ui.c`
- `components/blusys_services/include/blusys/display/lcd.h`
- `components/blusys_services/include/blusys/display/ui.h`
- `examples/lcd_st7735_basic/main/lcd_st7735_basic_main.c`
- `examples/lcd_st7735_basic/main/Kconfig.projbuild`
- `examples/ui_basic/main/ui_basic_main.c`
- `examples/ui_basic/main/Kconfig.projbuild`
- `examples/ui_basic/main/idf_component.yml`

## High-Level Structure

- `blusys_lcd_*` is the hardware-facing display service built on ESP-IDF `esp_lcd`.
- `blusys_ui_*` is a higher-level LVGL integration layer that sits on top of an already-opened `blusys_lcd_t *`.
- The UI service does not open the panel itself. The application opens LCD first, then passes that handle into `blusys_ui_open()`.

## LCD Layer

The LCD service lives in `components/blusys_services/src/display/lcd.c`.

- Supported SPI panel drivers:
  - `BLUSYS_LCD_DRIVER_ST7789`
  - `BLUSYS_LCD_DRIVER_ST7735`
  - `BLUSYS_LCD_DRIVER_NT35510`
- Supported I2C panel driver:
  - `BLUSYS_LCD_DRIVER_SSD1306`

The internal `blusys_lcd` struct stores:

- ESP-IDF panel IO handle
- ESP-IDF panel handle
- internal lock
- optional backlight GPIO pin
- active SPI or I2C bus ownership state
- cached panel width and height

### LCD Open Flow

`blusys_lcd_open()` does the following:

1. validates the config
2. allocates the LCD handle
3. initializes the internal lock
4. opens either SPI or I2C transport
5. resets the LCD panel
6. initializes the panel
   - for `ST7735`, this now uses a dedicated internal ST7735 panel backend with its
     own reset/init/draw/orientation implementation instead of reusing the ST7789
     panel object
7. applies panel gap offsets for SPI panels when configured
8. applies configured orientation and inversion
9. turns the display on
10. enables the backlight GPIO if configured
11. for ST7735 bitmap transfers, re-sends `COLMOD` before the draw

### LCD Thread Safety

- LCD operations are protected by an internal mutex.
- `blusys_lcd_draw_bitmap()` holds the lock while calling `esp_lcd_panel_draw_bitmap()`.
- Power and orientation operations also take the same lock.

### LCD Data Needed By UI

`blusys_ui_open()` depends on `blusys_lcd_get_dimensions()` to fetch the LCD width and height before creating the LVGL display.

## UI Layer

The UI service lives in `components/blusys_services/src/display/ui.c`.

- It is compiled only when `BLUSYS_HAS_LVGL` is defined.
- That define is set in `components/blusys_services/CMakeLists.txt` when the build includes the managed component `lvgl__lvgl`.
- The example project declares LVGL in `examples/ui_basic/main/idf_component.yml`.

## UI Lifetime Model

Expected order:

1. `blusys_lcd_open()`
2. `blusys_ui_open()`
3. create and update LVGL widgets
4. `blusys_ui_close()`
5. `blusys_lcd_close()`

The LCD must outlive the UI because the UI render task flushes pixels through the LCD handle until shutdown completes.

## UI Singleton Behavior

- Only one UI instance may be open at a time.
- `ui.c` enforces this with the static pointer `s_active_handle`.
- If another UI instance is opened while one is active, `blusys_ui_open()` returns `BLUSYS_ERR_INVALID_STATE`.

## UI Open Flow

`blusys_ui_open()` does the following:

1. validates `config`, `config->lcd`, and `out_ui`
2. rejects the call if another UI instance is already active
3. reads LCD dimensions from `blusys_lcd_get_dimensions()`
4. allocates a `blusys_ui` handle
5. creates a binary semaphore used during shutdown
6. initializes LVGL with `lv_init()`
7. sets LVGL tick source with `lv_tick_set_cb()` using `esp_timer_get_time()`
8. creates an LVGL display with the LCD width and height
9. stores the `blusys_ui_t *` as LVGL display user data
10. registers the LVGL flush callback
11. allocates two draw buffers
12. registers the buffers with LVGL in partial render mode
13. starts a FreeRTOS render task named `blusys_ui`
14. stores the instance as the active singleton and returns it

## LVGL Tick And Render Loop

### Tick Source

- `tick_get_cb()` returns elapsed milliseconds from `esp_timer_get_time()`.
- This callback is given to LVGL via `lv_tick_set_cb()`.

### Render Task

- `render_task()` runs while `ui->running` is true.
- It repeatedly calls `lv_timer_handler()`.
- The returned sleep interval is clamped to a minimum of 5 ms and a maximum of 500 ms.
- The task then delays for that duration.
- On shutdown, it signals `done_sem` and deletes itself.

## Flush Path

LVGL display flushes go through `flush_cb()`.

Current flow:

1. LVGL passes the dirty rectangle and pixel buffer to `flush_cb()`
2. `flush_cb()` gets the `blusys_ui_t *` from LVGL display user data
3. it reads the active LVGL draw-buffer stride
4. it byte-swaps each dirty row in place for RGB565 SPI transfer
5. it flushes the dirty area row-by-row with `blusys_lcd_draw_bitmap()`
   - for SPI panels, each LCD draw now waits until the color transfer completes
6. it calls `lv_display_flush_ready()` after the last row

The code comment says this swap is needed because SPI LCDs expect big-endian RGB565 while the ESP32 is little-endian.

## App-Side LVGL Access Model

- Application code is expected to call `blusys_ui_lock()` before using `lv_*` APIs from tasks other than the render task.
- Application code must then call `blusys_ui_unlock()` afterward.
- The example in `examples/ui_basic/main/ui_basic_main.c` follows that pattern when creating widgets.

Current example flow:

1. open an ST7735 LCD
2. open UI with that LCD
3. lock the UI
4. create widgets on `lv_screen_active()`
5. unlock the UI
6. keep the app alive while the render task updates the screen in the background

## Example Configuration Notes

`examples/ui_basic/main/ui_basic_main.c` currently uses:

- `BLUSYS_LCD_DRIVER_ST7735`
- width/height selected from orientation:
  - `160x128` when `swap_xy = true`
  - `128x160` when `swap_xy = false`
- RGB565 via `bits_per_pixel = 16`
- Kconfig-driven panel tuning:
  - `x_offset`
  - `y_offset`
  - `swap_xy`
  - `mirror_x`
  - `mirror_y`
  - `invert_color`

These details are important because color, alignment, and orientation issues may be specific to this panel setup.

## Build-Time LVGL Dependency

- `components/blusys_services/CMakeLists.txt` always builds `src/display/ui.c`.
- When LVGL is present, the component links the LVGL library and defines `BLUSYS_HAS_LVGL=1`.
- When LVGL is not present, `ui.c` builds fallback stubs that return `BLUSYS_ERR_NOT_SUPPORTED`.

## Current Assumptions And Potential Issue Areas

These are the main places likely to matter when debugging LVGL integration problems:

- `flush_cb()` unconditionally byte-swaps the flush buffer in place.
- The flush path assumes 16-bit pixel data.
- The ST7735 path now uses a dedicated internal panel backend; if raw drawing is still
  wrong after this change, the next suspects are panel-specific offsets, orientation
  tuning, or module-specific ST7735/ST7735-compatible quirks rather than inherited
  ST7789 ops.
- SPI LCD draws now wait for `on_color_trans_done()` before returning, so temporary
  draw buffers are safe to reuse after each `blusys_lcd_draw_bitmap()` call.
- The UI code uses LVGL global lock helpers `lv_lock()` and `lv_unlock()`.
- `blusys_ui_close()` stops the render task first, then deletes the LVGL display and deinitializes LVGL.
- LCD dimensions are cached in the LCD handle and returned directly by `blusys_lcd_get_dimensions()`.
- The remaining LVGL corruption was traced to `flush_cb()` assuming the rendered dirty
  area was always tightly packed; the working fix was to respect the active draw-buffer
  stride and flush row-by-row.

## Practical Summary

Blusys UI is a thin LVGL wrapper around the LCD service:

- LCD owns the hardware transport and panel driver
- UI owns LVGL initialization, display registration, draw buffers, and render task
- LVGL renders into buffers
- `flush_cb()` forwards those buffers to `blusys_lcd_draw_bitmap()`
- application tasks must lock around direct LVGL calls

This means most UI bugs will likely come from one of four places:

- LCD panel configuration
- flush callback behavior
- LVGL locking/tasking behavior
- panel-specific offsets or color ordering

## Current Status

After the latest round of fixes, the investigation stands here:

- The ST7735 service now has configurable inversion/orientation and a dedicated internal
  ST7735 backend instead of continued ST7789 reuse.
- SPI LCD transfers now honor the blocking contract of `blusys_lcd_draw_bitmap()`.
- The LVGL flush path now respects the active draw-buffer stride and flushes row-by-row.
- Hardware validation now confirms that both `examples/lcd_st7735_basic/` and
  `examples/ui_basic/` render correctly on the target ESP32-C3 + ST7735 setup.
