#ifndef BLUSYS_DISPLAY_H
#define BLUSYS_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/error.h"
#include "blusys/drivers/display/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_ui blusys_ui_t;

/* Panel pixel-format family. blusys_ui still renders LVGL in RGB565
 * internally regardless of choice; the difference is what flush_cb
 * sends to the panel:
 *
 *   RGB565    – byte-swap into the DMA scratch buffer and forward to
 *               blusys_lcd_draw_bitmap (default; matches ST7735 / ILI9341
 *               and other 16-bit SPI panels).
 *   RGB565_NATIVE – same LVGL RGB565 buffers, no byte swap (use for
 *               esp_lcd_qemu_rgb and any panel that expects LVGL's native
 *               16-bit order, see Espressif's lcd_qemu_rgb_panel example).
 *   MONO_PAGE – threshold each RGB565 pixel to 1 bit, pack into the
 *               SSD1306-style page format (each byte = 8 vertical
 *               pixels in a page, MSB at the bottom of the page),
 *               and forward the whole-screen page buffer to
 *               blusys_lcd_draw_bitmap. Requires the underlying LCD to
 *               have been opened with a 1-bpp driver
 *               (e.g. BLUSYS_LCD_DRIVER_SSD1306). Forces full-refresh
 *               mode internally because the page buffer is built whole
 *               from each LVGL flush area.
 *
 * Default value (zero-init) is RGB565 — preserves backwards
 * compatibility with every existing caller. */
typedef enum {
    BLUSYS_UI_PANEL_KIND_RGB565         = 0,
    BLUSYS_UI_PANEL_KIND_MONO_PAGE      = 1,
    BLUSYS_UI_PANEL_KIND_RGB565_NATIVE  = 2,
} blusys_ui_panel_kind_t;

typedef struct {
    /* Required: an already-opened LCD handle.
     *
     * The caller must keep the LCD open for the entire lifetime of the
     * blusys_ui instance.  Calling blusys_lcd_close() while blusys_ui is
     * running leaves the render task with a dangling pointer and will
     * produce undefined behaviour on the next flush.  Always call
     * blusys_ui_close() before blusys_lcd_close(). */
    blusys_lcd_t           *lcd;

    uint32_t                buf_lines;     /* Draw buffer height in lines (0 = default 20) */
    bool                    full_refresh;  /* Use one full-screen render buffer (experimental on SPI LCDs) */
    int                     task_priority; /* LVGL render task priority (0 = default 5) */
    int                     task_stack;    /* Render task stack in bytes (0 = default 16384) */
    blusys_ui_panel_kind_t  panel_kind;    /* Pixel-format family (default 0 = RGB565) */
} blusys_ui_config_t;

blusys_err_t blusys_ui_open(const blusys_ui_config_t *config,
                            blusys_ui_t **out_ui);
blusys_err_t blusys_ui_close(blusys_ui_t *ui);

/* Lock/unlock for thread-safe LVGL widget access from non-render tasks.
 *
 * These wrap LVGL's global lv_lock() / lv_unlock().  Because only one
 * blusys_ui instance may exist at a time (blusys_ui is a singleton) the
 * lock is effectively per-display, but callers from different tasks all
 * contend on the same underlying mutex.  Call blusys_ui_lock() before
 * creating, modifying, or deleting any LVGL object from outside the
 * render task, and release it as soon as possible — the lock is held by
 * the render task for the duration of each lv_timer_handler() call. */
blusys_err_t blusys_ui_lock(blusys_ui_t *ui);
blusys_err_t blusys_ui_unlock(blusys_ui_t *ui);

/* Batch UI construction: LVGL 9 must not queue invalidations while the display
 * is mid-refresh (see lv_refr.c / lv_inv_area). Building a deep dashboard tree
 * under the UI lock can otherwise trip assertions or wedge the main task on
 * slow MCUs. Call begin/end around theme + first screen assembly with the lock
 * held; end() re-enables invalidation and marks the active screen dirty. */
void blusys_ui_invalidation_begin(blusys_ui_t *ui);
void blusys_ui_invalidation_end(blusys_ui_t *ui);

#ifdef __cplusplus
}
#endif

#endif
