/**
 * @file display.h
 * @brief LVGL integration on top of an already-opened ::blusys_lcd_t panel.
 *
 * Only one ::blusys_display_t may exist at a time (LVGL's global state is a
 * singleton). The caller owns the underlying LCD handle and must close the
 * display before closing the LCD. See docs/hal/display.md.
 */

#ifndef BLUSYS_DISPLAY_H
#define BLUSYS_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"
#include "blusys/drivers/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to the singleton display instance. */
typedef struct blusys_display blusys_display_t;

/**
 * @brief Panel pixel-format family.
 *
 * Blusys renders LVGL in RGB565 internally regardless of this choice; the
 * difference is what `flush_cb` sends to the panel:
 *
 * - ::BLUSYS_DISPLAY_PANEL_KIND_RGB565 — byte-swap into the DMA scratch
 *   buffer and forward to ::blusys_lcd_draw_bitmap (default; matches
 *   ST7735 / ILI9341 and other 16-bit SPI panels).
 * - ::BLUSYS_DISPLAY_PANEL_KIND_RGB565_NATIVE — same RGB565 buffers, no
 *   byte swap (for `esp_lcd_qemu_rgb` and any panel that expects LVGL's
 *   native 16-bit order).
 * - ::BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE — threshold each RGB565 pixel
 *   to 1 bit and pack into SSD1306-style page format before forwarding
 *   the whole-screen buffer to ::blusys_lcd_draw_bitmap. Requires a 1-bpp
 *   LCD (e.g. ::BLUSYS_LCD_DRIVER_SSD1306) and forces full-refresh mode
 *   internally.
 */
typedef enum {
    BLUSYS_DISPLAY_PANEL_KIND_RGB565         = 0,  /**< 16-bpp SPI panel; LVGL byte-order swapped. */
    BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE      = 1,  /**< 1-bpp page-addressed OLED (SSD1306). */
    BLUSYS_DISPLAY_PANEL_KIND_RGB565_NATIVE  = 2,  /**< 16-bpp panel expecting LVGL's native byte order. */
} blusys_display_panel_kind_t;

/** @brief Configuration for ::blusys_display_open. */
typedef struct {
    /**
     * @brief Already-opened LCD handle (required).
     *
     * The caller must keep the LCD open for the entire lifetime of the
     * display instance. Closing the LCD while the display is running
     * leaves the render task with a dangling pointer. Always call
     * ::blusys_display_close before ::blusys_lcd_close.
     */
    blusys_lcd_t               *lcd;

    uint32_t                    buf_lines;     /**< Draw-buffer height in lines; `0` selects 20. */
    bool                        full_refresh; /**< Use one full-screen render buffer (experimental on SPI LCDs). */
    int                         task_priority; /**< LVGL render-task priority; `0` selects 5. */
    int                         task_stack;    /**< Render-task stack size in bytes; `0` selects 16384. */
    blusys_display_panel_kind_t panel_kind;    /**< Pixel-format family; default `0` = RGB565. */
} blusys_display_config_t;

/**
 * @brief Initialise LVGL, allocate render buffers, and start the render task.
 * @param config  Configuration.
 * @param out_ui  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_INVALID_STATE` if a display is already open.
 */
blusys_err_t blusys_display_open(const blusys_display_config_t *config,
                            blusys_display_t **out_ui);

/**
 * @brief Stop the render task and release the display handle.
 *
 * The caller is responsible for closing the underlying LCD afterwards.
 *
 * @param ui  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p ui is NULL.
 */
blusys_err_t blusys_display_close(blusys_display_t *ui);

/**
 * @brief Acquire the LVGL global lock for safe widget access.
 *
 * Wraps LVGL's `lv_lock()`. Because ::blusys_display_t is a singleton, this
 * lock is effectively per-display. Hold the lock while creating, modifying,
 * or deleting any LVGL object from outside the render task, and release it
 * as soon as possible.
 *
 * @param ui  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p ui is NULL.
 */
blusys_err_t blusys_display_lock(blusys_display_t *ui);

/**
 * @brief Release the LVGL global lock.
 * @param ui  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p ui is NULL.
 */
blusys_err_t blusys_display_unlock(blusys_display_t *ui);

/**
 * @brief Batch-build an LVGL subtree with invalidation suspended.
 *
 * LVGL 9 must not queue invalidations while a refresh is in progress. Call
 * @p begin before assembling a deep screen under the UI lock, and
 * ::blusys_display_invalidation_end when done; @p end re-enables invalidation
 * and marks the active screen dirty.
 *
 * @param ui  Handle.
 */
void blusys_display_invalidation_begin(blusys_display_t *ui);

/**
 * @brief Re-enable LVGL invalidation after a ::blusys_display_invalidation_begin.
 * @param ui  Handle.
 */
void blusys_display_invalidation_end(blusys_display_t *ui);

#ifdef __cplusplus
}
#endif

#endif
