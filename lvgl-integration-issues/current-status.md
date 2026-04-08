# LVGL Integration — Resolved

The ST7735 + LVGL corruption on ESP32-C3 is fixed. This file records the final root-cause analysis and solution for future reference.

## Root Causes

Two independent bugs combined to produce the corruption:

### 1. SPI DMA descriptor chaining (lcd.c)

ESP-IDF's SPI master driver requires a single DMA descriptor per transfer.  When a `RAMWR` payload exceeds `SPI_MAX_DMA_LEN` bytes the driver internally chains descriptors, and the ST7735 would misinterpret the chained burst as separate commands, producing sheared or duplicated output.

**Fix:** `blusys_st7735_panel_draw_bitmap()` now splits large bitmaps into row-aligned chunks that fit within `SPI_MAX_DMA_LEN`. Each chunk gets its own `COLMOD` + `CASET` + `RASET` + `RAMWR` sequence.

### 2. LVGL buffer stride vs. packed area width (ui.c)

LVGL's draw buffer has a `stride` that is often wider than the dirty area being flushed. The previous flush callback read row data using `packed_row_bytes` as the row pitch, which skipped bytes at the end of each row and fed misaligned pixel data to the LCD.

**Fix:** `flush_cb()` now reads rows using `draw_buf->header.stride` as the source pitch, copies them packed (area-width only) into a dedicated DMA-safe scratch buffer, byte-swaps, and calls `blusys_lcd_draw_bitmap()` once per band.

## Solution Summary

- `lcd.c` — `draw_bitmap` chunks to `SPI_MAX_DMA_LEN`; `COLMOD` re-asserted per chunk to prevent the ST7735 dropping its pixel format mid-session.
- `lcd.c` — SPI `max_transfer_sz` sized to full panel height instead of a fixed 80-line cap.
- `ui.c` — `flush_cb` copies LVGL rows (stride-pitched) into a packed DMA-safe scratch buffer in multi-row bands; byte-swaps the scratch buffer before each `draw_bitmap` call.
- `ui.c` — scratch buffer always allocated (not just in full-refresh mode).

## Validated Configuration

- Target: ESP32-C3 + ST7735 (160×128, landscape via `swap_xy=true`, `mirror_x=true`)
- SPI clock: 8 MHz (48 MHz also validated; wire time is no longer the bottleneck)
- LVGL mode: partial render, `buf_lines=20`, double-buffered
