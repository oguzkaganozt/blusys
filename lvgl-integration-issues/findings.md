# LVGL Integration Debug Findings

**Symptom** (confirmed from photo):
- **White background** instead of the intended black background.
- **"BluPanda" text renders as a vertical strip** of pixels instead of a horizontal label.

Both bugs are visible in the photo: the full display is white (inverted) and the text
appears as a garbled vertical column near the centre.

---

## Environment

- LVGL version: **9.2.2** (managed component `lvgl__lvgl`)
- ESP-IDF version: **v5.5.4**
- Target: **ESP32-C3**
- Panel: **ST7735**, 128×160, RGB565
- `CONFIG_LV_OS_FREERTOS=y`, `CONFIG_LV_COLOR_DEPTH_16=y`

---

## Bug 1 (Confirmed) — `invert_color(true)` Hardcoded for All ST7735 Panels

**File**: `components/blusys_services/src/display/lcd.c`, lines 292–294

```c
if (esp_err == ESP_OK && config->driver == BLUSYS_LCD_DRIVER_ST7735) {
    esp_err = esp_lcd_panel_invert_color(lcd->panel_handle, true);
}
```

`esp_lcd_panel_invert_color(true)` sends `INVON` (0x21) to the ST7735, which makes the
controller display the bitwise complement of every stored pixel value.

| LVGL renders | Sent over SPI | After INVON | Displayed |
|---|---|---|---|
| Background — black `0x0000` | `0x0000` | `~0x0000 = 0xFFFF` | **white** |
| Text — white `0xFFFF` | `0xFFFF` | `~0xFFFF = 0x0000` | **black** |

Hardware inversion is a per-module quirk depending on how the manufacturer wired the
panel glass. Hard-coding it for every ST7735 breaks modules that do not need it.

### Fix

Add an `invert_color` field to `blusys_lcd_config_t` and use it instead of the
driver-type check.

**`components/blusys_services/include/blusys/display/lcd.h`** — in `blusys_lcd_config_t`:

```c
bool invert_color;   /**< true if the module requires hardware colour inversion (INVON) */
```

**`components/blusys_services/src/display/lcd.c`** — replace:

```c
/* REMOVE: */
if (esp_err == ESP_OK && config->driver == BLUSYS_LCD_DRIVER_ST7735) {
    esp_err = esp_lcd_panel_invert_color(lcd->panel_handle, true);
}

/* ADD: */
if (esp_err == ESP_OK && config->invert_color) {
    esp_err = esp_lcd_panel_invert_color(lcd->panel_handle, true);
}
```

**`examples/ui_basic/main/ui_basic_main.c`**:

```c
blusys_lcd_config_t lcd_config = {
    ...
    .invert_color = false,   /* set true only if this specific module requires INVON */
    ...
};
```

---

## Bug 2 (Confirmed) — Wrong Display Orientation (Text Appears Vertical)

**Root cause**: The ESP-IDF `esp_lcd_new_panel_st7789()` driver initialises MADCTL = 0x00,
which sets:
- **MV = 0** (no row/column exchange)
- MX = 0, MY = 0

For ST7789 panels this gives correct portrait mode. For many ST7735 modules the glass is
physically mounted with the row/column scan direction rotated 90° relative to ST7789.
With MV=0 the controller writes each of LVGL's *horizontal* pixel rows into a *vertical*
column of the physical glass.

Result: a label centred horizontally in a 128×160 portrait canvas appears as a
vertical strip of pixels near the horizontal centre of the display. This matches the
photo exactly.

### How swap_xy fixes it

`blusys_lcd_swap_xy(lcd, true)` calls `esp_lcd_panel_swap_xy()`, which sets
**MV = 1** in MADCTL (bit 5). With MV=1 the controller's column/row addressing is
transposed, so LVGL's horizontal rows map to physical rows on the glass, producing
correct portrait output.

Depending on the exact module a mirror step may also be needed:
- `blusys_lcd_mirror(lcd, true, false)` sets MX=1 (flip columns left↔right).
- `blusys_lcd_mirror(lcd, false, true)` sets MY=1 (flip rows top↔bottom).
- Common portrait fix for 128×160 ST7735 modules: **MV=1 + MX=1** (MADCTL = 0x60).

### Fix — add orientation fields to blusys_lcd_config_t

The cleanest solution is to apply orientation at init time via config, avoiding the need
for the caller to issue post-open API calls.

**`components/blusys_services/include/blusys/display/lcd.h`**:

```c
typedef struct {
    blusys_lcd_driver_t driver;
    uint32_t            width;
    uint32_t            height;
    uint8_t             bits_per_pixel;
    bool                bgr_order;
    bool                invert_color;  /* NEW: hardware colour inversion */
    bool                swap_xy;       /* NEW: transpose row/column (MADCTL MV) */
    bool                mirror_x;      /* NEW: flip columns (MADCTL MX) */
    bool                mirror_y;      /* NEW: flip rows    (MADCTL MY) */
    union {
        blusys_lcd_spi_config_t spi;
        blusys_lcd_i2c_config_t i2c;
    };
} blusys_lcd_config_t;
```

**`components/blusys_services/src/display/lcd.c`** — after `esp_lcd_panel_disp_on_off(true)` succeeds, before the backlight step:

```c
if (esp_err == ESP_OK && config->swap_xy) {
    esp_err = esp_lcd_panel_swap_xy(lcd->panel_handle, true);
}
if (esp_err == ESP_OK && (config->mirror_x || config->mirror_y)) {
    esp_err = esp_lcd_panel_mirror(lcd->panel_handle,
                                   config->mirror_x, config->mirror_y);
}
if (esp_err == ESP_OK && config->invert_color) {
    esp_err = esp_lcd_panel_invert_color(lcd->panel_handle, true);
}
```

**`examples/ui_basic/main/ui_basic_main.c`** — try this combination first for the
common 128×160 ST7735 module (MV=1, MX=1 = MADCTL 0x60):

```c
blusys_lcd_config_t lcd_config = {
    .driver         = BLUSYS_LCD_DRIVER_ST7735,
    .width          = LCD_WIDTH,    /* 128 */
    .height         = LCD_HEIGHT,   /* 160 */
    .bits_per_pixel = 16,
    .bgr_order      = false,
    .invert_color   = false,
    .swap_xy        = true,
    .mirror_x       = true,
    .mirror_y       = false,
    .spi = { ... },
};
```

If the image is still mirrored or flipped after this, toggle `mirror_x` / `mirror_y`
one at a time. There are four possible MV+MX+MY combinations for portrait; the correct
one depends on the specific PCB layout.

---

## Bug 3 — ST7789 Driver Sends ST7789-Specific RAMCTRL (0xB0) to ST7735

`esp_lcd_new_panel_st7789()` is used for both ST7789 and ST7735. The init sequence sends
command **0xB0 (RAMCTRL)** — an ST7789-only register that configures pixel data endianness.
ST7735 does not define 0xB0 and may ignore or misinterpret it.

In practice the current display still works (basic commands CASET/RASET/RAMWR are shared).
But this is the correct long-term reason to add a separate ST7735 init path.

**Status**: not causing either current symptom; medium-priority robustness issue.

---

## Bug 4 — LVGL Color Format Not Explicitly Set

`lv_display_create()` defaults to `LV_COLOR_FORMAT_NATIVE`, which resolves correctly to
`LV_COLOR_FORMAT_RGB565` when `CONFIG_LV_COLOR_DEPTH_16=y`. But this is an implicit
dependency on the sdkconfig value.

**Fix** — add one line after `lv_display_create()` in `blusys_ui_open()`:

```c
lv_display_set_color_format(ui->disp, LV_COLOR_FORMAT_RGB565);
```

**Status**: defensive hardening, not causing either current symptom.

---

## Non-Issues Confirmed

### Byte swap in flush_cb

The in-place RGB565 byte swap is correct and safe with double-buffering. LVGL renders
into the other buffer while DMA reads the current one. The `LV_COLOR_16_SWAP` backward-
compat path in LVGL v9 is not active in this build so there is no double-swap.

### flush_ready before DMA completes

Calling `lv_display_flush_ready()` before DMA finishes is intentional. LVGL renders the
next strip into the alternate buffer; the following `esp_lcd_panel_draw_bitmap()` call
blocks on `trans_queue_depth=1` until the previous DMA completes before starting the next.

### lv_timer_handler() locking

`lv_timer_handler()` acquires `lv_lock()` internally. The render task calls it without
holding the lock (correct pattern). App tasks call `blusys_ui_lock()` /
`blusys_ui_unlock()` to serialise with the render task via the same FreeRTOS mutex.

---

## Render Task Stack — Precautionary Note

`UI_DEFAULT_TASK_STACK = 8192` bytes. LVGL v9.2 with FreeRTOS locking, font
rasterization, and the SPI flush frame may exceed this on complex UIs. The commonly
recommended minimum for LVGL v9 on ESP32 targets is **16 384 bytes**. Raise it
proactively to avoid intermittent crashes under heavier load.

---

## Prioritised Fix List

| # | Files | Change | Priority |
|---|-------|---------|----------|
| 1 | `lcd.h` + `lcd.c` | Add `swap_xy`, `mirror_x`, `mirror_y`, `invert_color` to config; apply in `lcd_open()` | **Critical — fixes both visible bugs** |
| 2 | `ui_basic_main.c` | Set `.swap_xy=true, .mirror_x=true, .invert_color=false` (tune mirrors if needed) | **Critical — fixes both visible bugs** |
| 3 | `ui.c` | Raise `UI_DEFAULT_TASK_STACK` to `16384` | High (preventive) |
| 4 | `ui.c` | Add `lv_display_set_color_format(ui->disp, LV_COLOR_FORMAT_RGB565)` | Medium (hardening) |
| 5 | `lcd.c` | Separate ST7735 init path to avoid sending RAMCTRL (0xB0) | Medium (robustness) |

---

## Follow-up Status After Driver Changes

The items above were the correct first wave of fixes, but follow-up hardware testing
shows that they do **not** fully solve the ST7735 path.

### Improvements Confirmed

- The forced-white background problem is fixed. Removing the hard-coded ST7735
  inversion and making inversion configurable stopped the panel from always showing
  black pixels as white.
- The original LVGL symptom where `"BluPanda"` collapsed into a vertical strip is
  fixed. Init-time `swap_xy` / `mirror_x` / `mirror_y` control made the panel usable
  enough to continue debugging.
- The gross wraparound / staircase corruption seen in early landscape tests improved
  after moving work into the LCD service and adding ST7735-specific initialization.

### Additional Changes Implemented

These code changes were made after the initial findings above:

- `blusys_lcd_config_t` now includes:
  - `swap_xy`
  - `mirror_x`
  - `mirror_y`
  - `invert_color`
- `blusys_lcd_open()` applies panel gap, orientation, and inversion from config.
- `examples/ui_basic/` and `examples/lcd_st7735_basic/` expose ST7735 tuning via
  `Kconfig.projbuild` instead of hard-coded values only.
- `examples/ui_basic/` gained a debug border/corner overlay for calibration.
- `examples/ui_basic/` and `examples/lcd_st7735_basic/` were updated to use
  orientation-aware width/height when `swap_xy` is enabled.
- The ST7735 service path now sends a dedicated ST7735 init sequence instead of
  relying only on the ST7789 init defaults.
- The ST7735 path now re-sends `COLMOD` before each transfer, following behavior used
  in other working ST7735 drivers.
- ST7735 RGB565 `COLMOD` was changed from `0x55` to `0x05`, which matches common
  ST7735R/S SPI init sequences.

### New Confirmed Finding — The Remaining Bug Is Below LVGL

After the LCD-service fixes above, `examples/lcd_st7735_basic/` still renders a
stable dark background but corrupts the `"BluPanda"` text. This is critical because
that example does **not** use LVGL at all.

That means:

- the remaining corruption is **not** caused by `ui.c`
- it is **not** caused by LVGL locking or tasking
- it is **not** caused by LVGL's flush callback

The remaining issue is now clearly in the **ST7735 LCD service path**.

### What The Raw Example Suggests

The raw ST7735 example currently shows this pattern:

- full-screen or row-by-row fills are stable
- small multi-row glyph rectangles are still corrupted
- the corruption persists even when the pixels are only `0x0000` and `0xFFFF`

This strongly suggests the remaining bug is **not** simple color endianness. Black
and white are invariant under byte swap, yet the glyph geometry is still wrong.

The most likely remaining causes are:

- ST7735 address-window behavior under rotation (`MADCTL MV`) does not match the
  ST7789 draw logic that is still being reused
- rotation-dependent start offsets/gaps are still wrong for this board
- the panel is a clone or close variant marketed as ST7735 (for example GC9106-like
  behavior), so the expected ST7735 register model is only partially correct

### Updated Conclusion

The investigation should now prioritise the LCD service, not the LVGL example.

The current ST7735 code path is still structurally compromised because:

- panel allocation still goes through `esp_lcd_new_panel_st7789()`
- only the init sequence has been partially specialized for ST7735
- draw-window and RAM-write behavior still come from ST7789-oriented ops

This is now the highest-probability source of the remaining corruption.

### Updated Next Step

The next proper fix is:

1. Implement a **dedicated ST7735 panel backend** instead of reusing the ST7789 panel
   object.
2. Validate that backend first with `examples/lcd_st7735_basic/`.
3. Only after raw drawing is correct, re-check `examples/ui_basic/`.

If a short-term fallback is needed, portrait-only support is acceptable temporarily.
But long-term, the driver should support the rotated addressing modes that the ST7735
controller exposes.

### Implementation Status After This Round

The first item above is now implemented in the codebase:

- `components/blusys_services/src/display/lcd.c` now creates ST7735 panels through a
  dedicated internal backend instead of `esp_lcd_new_panel_st7789()`.
- That backend now owns ST7735 reset, init, draw-window setup, MADCTL updates, gap
  handling, inversion, and display on/off.
- ST7735 draw-window gaps now follow the active `swap_xy` state, matching the
  rotation-dependent offset handling used by known-good ST7735 libraries.
- `examples/lcd_st7735_basic/` now places the `"BluPanda"` text using the active
  logical width/height instead of stale hard-coded coordinates.
- `components/blusys_services/src/display/ui.c` now explicitly selects
  `LV_COLOR_FORMAT_RGB565` and raises the default UI task stack to `16384`.
- SPI LCD transfers now wait for `on_color_trans_done()` before
  `blusys_lcd_draw_bitmap()` returns, restoring the documented blocking contract of
  the LCD API.
- Build verification passed for:
  - `blusys build examples/lcd_st7735_basic esp32c3`
  - `blusys build examples/ui_basic esp32c3`

### New Confirmed Finding From The Latest Hardware Photo

The latest `lcd_st7735_basic` photo changes the diagnosis again:

- the text is now horizontal and centered, so the dedicated ST7735 backend fixed the
  earlier address-window / orientation failure mode
- the remaining corruption pattern matches **buffer lifetime misuse** rather than
  another ST7735 register issue

Why this matters:

- ESP-IDF documents `esp_lcd_panel_io_tx_color()` as asynchronous for SPI and requires
  the color buffer to stay valid until `on_color_trans_done()`
- the raw example reused a single glyph buffer for back-to-back character draws
- the LCD API docs already described `blusys_lcd_draw_bitmap()` as blocking, but the
  implementation was still returning before SPI color DMA had actually completed

That means the photo is consistent with the LCD service violating its own API contract,
not with another remaining ST7735 orientation bug.

### Final Resolved Status

Hardware validation now confirms that **both examples are working correctly**:

- `examples/lcd_st7735_basic/` renders `"BluPanda"` correctly.
- `examples/ui_basic/` now renders the debug overlay and LVGL label correctly.

Final resolution summary:

- The ST7735 path required a dedicated backend instead of continued ST7789 panel reuse.
- The LCD service needed to restore the documented blocking behavior of
  `blusys_lcd_draw_bitmap()` by waiting for SPI color transfer completion.
- The LVGL integration needed a flush-path fix: `flush_cb()` now respects the active
  LVGL draw-buffer stride and flushes row-by-row instead of assuming the rendered dirty
  area is always tightly packed in memory.

Final conclusion:

- The original visible failures were a combination of ST7735 panel configuration issues
  and a separate LVGL flush-buffer interpretation bug.
- The LCD layer and the LVGL wrapper are now both functioning correctly for this
  ESP32-C3 + ST7735 setup.
