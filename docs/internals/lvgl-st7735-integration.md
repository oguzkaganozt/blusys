# LVGL + ST7735 (ESP32-C3)

## Root cause (diagonal shear / slanted text)

LVGL’s flush path and row stride were **correct** (`lv_draw_buf_width_to_stride()` matches LVGL 9’s partial-buffer reshape). The artifact came from the **panel transfer shape**: packing **several scanlines** into one **`RASET` window** and **one long `RAMWR`** confused some ST7735 modules on SPI, producing shear even when the byte count per row was right.

## Fix (HAL)

`blusys_st7735_panel_draw_bitmap()` in `components/blusys_hal/src/drivers/display/lcd.c` sends **one display row per `RAMWR`** (each row: `COLMOD` + `CASET` + `RASET` + `RAMWR`). Throughput is lower than multi-row bursts; override in a fork only if you validate hardware.

`COLMOD` is still re-asserted per transaction and once more after the last `RAMWR` so DMA completion is synchronized before the next use of the source buffer.

## LVGL flush (`components/blusys_services/src/display/ui.c`)

Partial-mode `flush_cb` copies each dirty region from `px_map` using `lv_draw_buf_width_to_stride(area_width, cf)`, packs into the DMA scratch buffer, optionally byte-swaps RGB565 for SPI, and calls `blusys_lcd_draw_bitmap()`. `lv_display_flush_ready()` runs **after** all SPI/DMA for that flush completes.

## Notes

- SPI default for `st7735_160x128()` is **16 MHz**; lower `pclk_hz` in product integration if wiring is marginal.
- Typical **x/y gap** offsets for 160×128 modules remain in the profile; tune per glass if needed.
