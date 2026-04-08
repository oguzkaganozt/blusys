# LVGL Integration Current Status

This file is the canonical current-state summary for the ST7735 + LVGL investigation.
Older split notes in this directory were consolidated into this file and removed.

## Scope

This investigation is about the LVGL UI path in `examples/ui_basic/` on the ESP32-C3 + ST7735 setup.
The raw LCD path in `examples/lcd_st7735_basic/` is now mainly a known-good reference for what fast and correct drawing should look like.

## Relevant Components

- `components/blusys_services/src/display/lcd.c`
  The hardware-facing LCD service.
- `components/blusys_services/src/display/ui.c`
  The LVGL integration layer on top of an already-opened LCD handle.
- `examples/lcd_st7735_basic/`
  Raw LCD validation example.
- `examples/ui_basic/`
  LVGL validation example.

## Architecture Summary

- The LCD service owns the panel driver, transport, orientation, inversion, and bitmap writes.
- The UI service owns LVGL initialization, draw buffers, the render task, and the flush callback.
- Applications open the LCD first, then pass the LCD handle into `blusys_ui_open()`.
- LVGL renders into memory buffers, and `flush_cb()` forwards those pixels to `blusys_lcd_draw_bitmap()`.

## What Is Already Fixed

The major correctness bugs from the earlier rounds are fixed:

- ST7735 inversion is no longer hard-coded.
- Orientation is configurable with `swap_xy`, `mirror_x`, and `mirror_y`.
- The ST7735 path now uses a dedicated internal backend instead of reusing the ST7789 object.
- SPI LCD transfers now wait for `on_color_trans_done()`, so `blusys_lcd_draw_bitmap()` is blocking again as documented.
- `blusys_ui_open()` explicitly sets `LV_COLOR_FORMAT_RGB565`.
- The default LVGL render task stack is raised to `16384`.

## Current Verified State

### Raw LCD Path

`examples/lcd_st7735_basic/` is fast and correct on the target hardware.

What this proves:

- the ST7735 backend is now fundamentally correct
- larger multi-row bitmap writes can work correctly
- the LCD transport is not inherently limited to one-row transfers

### LVGL UI Path

`examples/ui_basic/` is correct only with the conservative LVGL partial flush path.

Current safe behavior in `components/blusys_services/src/display/ui.c`:

- partial render mode is the only hardware-validated safe LVGL mode
- `flush_cb()` byte-swaps RGB565 per dirty row
- `flush_cb()` calls `blusys_lcd_draw_bitmap()` once per row
- `full_refresh` support still exists, but it remains experimental and disabled by default

## How The Screen Looks Now

With the safe path, the final image is correct:

- black background
- white edge border
- colored corner markers
- centered white `"BluPanda"` label

What still looks wrong is the transition:

- the screen visibly clears first
- the new content appears progressively
- the whole update takes roughly `1 second`

So the current problem is no longer final-frame corruption in the safe path. The current problem is visible redraw latency.

## What The Broken Accelerated Path Looked Like

The experimental accelerated LVGL paths produced corruption on hardware, including:

- diagonal slash-like artifacts
- split or duplicated `"BluPanda"` text
- band-boundary-looking corruption

That corrupted appearance is not the current safe-path result. The current safe path is correct but slow.

## Current Example Configuration

The current example defaults are:

- `examples/ui_basic/main/Kconfig.projbuild`
- `CONFIG_BLUSYS_UI_PCLK_HZ = 48000000`
- `CONFIG_BLUSYS_UI_FULL_REFRESH = n`
- `CONFIG_BLUSYS_UI_SWAP_XY = y`
- `CONFIG_BLUSYS_UI_MIRROR_X = y`
- `CONFIG_BLUSYS_UI_MIRROR_Y = n`
- `CONFIG_BLUSYS_UI_INVERT_COLOR = n`

The latest user test reported that increasing SPI from `16 MHz` to `48 MHz` made little to no visible difference.

## What That Means

For a `160x128` RGB565 frame:

- total payload is `40,960` bytes
- ideal wire time is about `20.5 ms` at `16 MHz`
- ideal wire time is about `6.8 ms` at `48 MHz`

That means the clock increase only saves about `13.7 ms` of ideal transfer time for a full-frame payload.
If the user still sees a redraw that takes around `1 second`, then the dominant cost is not raw SPI bandwidth.

## Current Bottleneck Hypothesis

The most likely bottleneck is the granularity of the safe LVGL flush path.

Why:

- `flush_cb()` currently flushes one LCD row at a time
- a full-screen invalidation on a `160x128` display means about `128` LCD draw calls just for the main screen area
- every row flush repeats lock, address-window setup, transfer submission, and completion wait
- for ST7735, the LCD path also re-sends `COLMOD` before each draw

The likely contributors are:

- too many tiny LCD transactions
- per-row byte-swapping overhead
- repeated LCD command/setup overhead
- waiting for DMA completion after every one-row transfer

The things that are no longer leading suspects for the current slow-but-correct state are:

- orientation
- inversion
- raw ST7735 backend correctness
- basic SPI clock rate

## Important Evidence From The LCD Layer

The LCD SPI transport is already configured for larger transfers.

In `components/blusys_services/src/display/lcd.c`, SPI is opened with:

- `.max_transfer_sz = width * 80 lines * bits_per_pixel`

And in `examples/lcd_st7735_basic/`, the screen is cleared in `40`-line bands.

This strongly suggests the current speed problem is not a hardware inability to transfer larger bands. It is a UI flush-shaping problem.

## Current Conclusion

The investigation has narrowed to this:

- the LCD backend is now good
- the safe LVGL path is correct but too fine-grained
- the accelerated LVGL paths tested so far are faster in principle but still corrupt

So the real question is:

- how to batch LVGL dirty rectangles into larger safe LCD transfers without reintroducing corruption

## Best Candidate Fixes

### 1. Instrumented Partial-Mode Band Flush

This is the best next step.

Approach:

- keep LVGL in partial render mode
- copy `N` rows at a time into a dedicated contiguous scratch buffer
- byte-swap during the copy into that buffer
- flush one multi-row band per scratch-buffer fill

Why this is promising:

- it reduces the number of LCD draw calls
- it stays close to the already-validated partial LVGL pipeline
- it mirrors the known-good strategy already used in `lcd_st7735_basic/`

### 2. Add Better Flush Instrumentation

Before changing transfer shapes blindly, log:

- flush areas
- active stride values
- packed row bytes
- chosen band ranges

This should make it easier to tell whether corruption comes from:

- LVGL-to-band repacking
- byte swapping
- LCD writes

### 3. Add A Debug Pattern For Flush Testing

Useful dedicated patterns:

- numbered horizontal bands
- checkerboards
- text placed near expected band boundaries

This would be much easier to reason about than only looking at the final demo UI.

### 4. Tune `buf_lines` Only After Safe Batching Exists

Current default in `ui.c`:

- `UI_DEFAULT_BUF_LINES = 20`

Larger partial buffers may help later, but not while the flush still degenerates into one-row LCD calls.

### 5. Revisit `full_refresh` Later, Not First

The existing experimental `full_refresh` path still corrupts.
It should not be the next focus until the safer partial-band path is instrumented and understood.

## Recommended Next Step

The recommended next sequence is:

1. Keep the current row-by-row partial flush as the correctness baseline.
2. Add instrumentation to `flush_cb()` on real hardware.
3. Implement a guarded partial-band scratch-buffer path.
4. Start with conservative band heights such as `20` or `40` lines.
5. Compare hardware results directly against the raw LCD example behavior.

## Bottom Line

The current situation is:

- correctness is restored in the safe LVGL path
- speed is still poor
- SPI clock alone does not solve it
- the unsolved problem is flush batching, not raw transport bring-up

That makes the next task a performance-shaping problem inside `components/blusys_services/src/display/ui.c`, not another panel-orientation or color-order bug hunt.
