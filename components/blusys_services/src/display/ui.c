#include "blusys/display/ui.h"

#include <stdlib.h>
#include <string.h>

#if defined(BLUSYS_HAS_LVGL)

#include "lvgl.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "blusys/internal/blusys_esp_err.h"

static const char *TAG = "blusys_ui";

#define UI_DEFAULT_TASK_PRIORITY  5
#define UI_DEFAULT_TASK_STACK     16384
#define UI_DEFAULT_BUF_LINES      20
#define UI_FLUSH_BAND_LINES       40

static blusys_ui_t *s_active_handle;

struct blusys_ui {
    blusys_lcd_t           *lcd;
    lv_display_t           *disp;
    void                   *buf1;
    void                   *buf2;
    void                   *flush_buf;
    size_t                  flush_buf_size;
    TaskHandle_t            task;
    SemaphoreHandle_t       done_sem;
    volatile bool           running;
    blusys_ui_panel_kind_t  panel_kind;
    uint32_t                panel_width;
    uint32_t                panel_height;
};

/* ------------------------------------------------------------------ */
/* LVGL tick source — returns elapsed ms via esp_timer                 */
/* ------------------------------------------------------------------ */
static uint32_t tick_get_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/* ------------------------------------------------------------------ */
/* MONO_PAGE flush — RGB565 → SSD1306 page format → blusys_lcd          */
/* ------------------------------------------------------------------ */
/* Threshold each RGB565 pixel from the LVGL flush area to 1 bit (any
 * non-black pixel = on), then pack into the SSD1306 native page
 * format: each byte represents 8 vertical pixels in a horizontal page,
 * MSB at the bottom of the page, 4 pages × width bytes for a 128x32
 * panel.
 *
 * Caller has set MONO_PAGE panel_kind, which forces full-refresh mode
 * during open. The flush area therefore always covers the entire
 * panel, and `flush_buf` is sized as panel_width * panel_height / 8
 * bytes — exactly the SSD1306 frame buffer size, so we can write the
 * page bytes directly into it. */
static void flush_cb_mono_page(blusys_ui_t *ui, const lv_area_t *area,
                               uint8_t *px_map)
{
    const uint16_t *src = (const uint16_t *)px_map;
    const int32_t   x1  = area->x1;
    const int32_t   y1  = area->y1;
    const int32_t   x2  = area->x2;
    const int32_t   y2  = area->y2;
    const int32_t   w   = x2 - x1 + 1;

    memset(ui->flush_buf, 0, ui->flush_buf_size);

    uint8_t *dst = (uint8_t *)ui->flush_buf;
    for (int32_t y = y1; y <= y2; ++y) {
        const int32_t page    = y / 8;
        const int32_t bit_idx = y % 8;
        const uint8_t bit     = (uint8_t)(1u << bit_idx);
        const uint16_t *row   = src + ((size_t)(y - y1) * (size_t)w);

        for (int32_t x = x1; x <= x2; ++x) {
            if (row[x - x1] != 0u) {
                dst[(size_t)page * (size_t)ui->panel_width + (size_t)x] |= bit;
            }
        }
    }

    blusys_lcd_draw_bitmap(ui->lcd,
                           0, 0,
                           (int32_t)ui->panel_width,
                           (int32_t)ui->panel_height,
                           ui->flush_buf);
}

/* ------------------------------------------------------------------ */
/* LVGL flush callback — forwards to blusys_lcd_draw_bitmap()          */
/* ------------------------------------------------------------------ */
/* Copy each LVGL flush area into a DMA-safe scratch buffer packed by area
 * width (not LVGL draw-buffer stride), byte-swap in place, then send to the
 * panel in multi-row bands. */
static void flush_cb(lv_display_t *disp, const lv_area_t *area,
                     uint8_t *px_map)
{
    blusys_ui_t *ui = lv_display_get_user_data(disp);

    if (ui->panel_kind == BLUSYS_UI_PANEL_KIND_MONO_PAGE) {
        flush_cb_mono_page(ui, area, px_map);
        lv_display_flush_ready(disp);
        return;
    }

    lv_draw_buf_t *draw_buf = lv_display_get_buf_active(disp);
    uint32_t area_width       = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t area_height      = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t pixel_size       = lv_color_format_get_size(lv_display_get_color_format(disp));
    uint32_t packed_row_bytes = area_width * pixel_size;
    uint32_t draw_stride      = packed_row_bytes;

    if ((draw_buf != NULL) && (draw_buf->header.stride != 0u)) {
        draw_stride = draw_buf->header.stride;
    }
    if (draw_stride < packed_row_bytes) {
        draw_stride = packed_row_bytes;
    }

    if ((ui->flush_buf != NULL) && (ui->flush_buf_size >= packed_row_bytes)) {
        uint32_t max_band_rows = (uint32_t)(ui->flush_buf_size / packed_row_bytes);
        if (max_band_rows == 0u) {
            max_band_rows = 1u;
        }

        for (uint32_t row = 0; row < area_height; row += max_band_rows) {
            uint32_t band_rows = area_height - row;
            if (band_rows > max_band_rows) {
                band_rows = max_band_rows;
            }

            for (uint32_t r = 0; r < band_rows; r++) {
                uint8_t *dst = (uint8_t *)ui->flush_buf + ((size_t)r * packed_row_bytes);
                const uint8_t *src = px_map + ((size_t)(row + r) * draw_stride);
                memcpy(dst, src, packed_row_bytes);
            }
            lv_draw_sw_rgb565_swap(ui->flush_buf, area_width * band_rows);

            blusys_lcd_draw_bitmap(ui->lcd,
                                   area->x1,
                                   area->y1 + (int32_t)row,
                                   area->x2 + 1,
                                   area->y1 + (int32_t)(row + band_rows),
                                   ui->flush_buf);
        }
    } else {
        /* Fallback when scratch buffer is unavailable: send row-by-row
         * directly from the LVGL buffer with an in-place byte-swap. */
        for (uint32_t row = 0; row < area_height; row++) {
            uint8_t *row_ptr = px_map + ((size_t)row * draw_stride);
            lv_draw_sw_rgb565_swap(row_ptr, area_width);
            blusys_lcd_draw_bitmap(ui->lcd,
                                   area->x1,
                                   area->y1 + (int32_t)row,
                                   area->x2 + 1,
                                   area->y1 + (int32_t)row + 1,
                                   row_ptr);
        }
    }

    lv_display_flush_ready(disp);
}

/* ------------------------------------------------------------------ */
/* Render task — calls lv_timer_handler() in a loop                    */
/* ------------------------------------------------------------------ */
static void render_task(void *arg)
{
    blusys_ui_t *ui = arg;

    while (ui->running) {
        uint32_t ms = lv_timer_handler();
        if (ms > 500) {
            ms = 500;
        }
        if (ms < 5) {
            ms = 5;
        }
        vTaskDelay(pdMS_TO_TICKS(ms));
    }

    xSemaphoreGive(ui->done_sem);
    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */
blusys_err_t blusys_ui_open(const blusys_ui_config_t *config,
                            blusys_ui_t **out_ui)
{
    if (config == NULL || config->lcd == NULL || out_ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (s_active_handle != NULL) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    uint32_t width = 0, height = 0;
    blusys_err_t err = blusys_lcd_get_dimensions(config->lcd, &width, &height);
    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_ui_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }
    ui->lcd          = config->lcd;
    ui->panel_kind   = config->panel_kind;
    ui->panel_width  = width;
    ui->panel_height = height;

    ui->done_sem = xSemaphoreCreateBinary();
    if (ui->done_sem == NULL) {
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }

    /* LVGL core init */
    lv_init();
    lv_tick_set_cb(tick_get_cb);

    /* Create display */
    ui->disp = lv_display_create((int32_t)width, (int32_t)height);
    if (ui->disp == NULL) {
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }
    lv_display_set_color_format(ui->disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(ui->disp, ui);
    lv_display_set_flush_cb(ui->disp, flush_cb);

    /* Allocate draw buffers.
     *
     * MONO_PAGE forces full-refresh because the SSD1306-style page
     * buffer is rebuilt whole on each flush — partial refresh would
     * leave the unupdated parts of the page buffer stale. The flush
     * scratch is sized to one full screen of page bytes
     * (width * height / 8), which is the SSD1306 frame buffer size. */
    bool full_refresh = config->full_refresh ||
                        (ui->panel_kind == BLUSYS_UI_PANEL_KIND_MONO_PAGE);
    uint32_t buf_lines = config->buf_lines > 0
                         ? config->buf_lines
                         : UI_DEFAULT_BUF_LINES;
    lv_color_format_t cf = lv_display_get_color_format(ui->disp);
    uint32_t stride = lv_draw_buf_width_to_stride(width, cf);
    uint32_t buf_bytes = stride * (full_refresh ? height : buf_lines);
    uint32_t flush_band_lines = (buf_lines > UI_FLUSH_BAND_LINES)
                                ? buf_lines
                                : UI_FLUSH_BAND_LINES;
    if (flush_band_lines > height) {
        flush_band_lines = height;
    }
    uint32_t flush_buf_bytes;
    if (ui->panel_kind == BLUSYS_UI_PANEL_KIND_MONO_PAGE) {
        flush_buf_bytes = width * height / 8u;
    } else {
        flush_buf_bytes = stride * flush_band_lines;
    }

    ui->buf1 = malloc(buf_bytes);
    ui->buf2 = full_refresh ? NULL : malloc(buf_bytes);
    ui->flush_buf = heap_caps_malloc(flush_buf_bytes,
                                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (ui->buf1 == NULL ||
        (!full_refresh && ui->buf2 == NULL) ||
        (ui->flush_buf == NULL)) {
        free(ui->buf1);
        free(ui->buf2);
        heap_caps_free(ui->flush_buf);
        lv_display_delete(ui->disp);
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }
    ui->flush_buf_size = flush_buf_bytes;
    lv_display_set_buffers(ui->disp, ui->buf1, ui->buf2,
                           buf_bytes,
                           full_refresh ? LV_DISPLAY_RENDER_MODE_FULL
                                        : LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG,
             "ui open: %lux%lu buf_lines=%lu buf_bytes=%lu scratch=%lu full_refresh=%d panel=%s",
             (unsigned long)width,
             (unsigned long)height,
             (unsigned long)buf_lines,
             (unsigned long)buf_bytes,
             (unsigned long)flush_buf_bytes,
             full_refresh ? 1 : 0,
             ui->panel_kind == BLUSYS_UI_PANEL_KIND_MONO_PAGE ? "mono_page" : "rgb565");

    /* Start render task */
    int prio  = config->task_priority > 0 ? config->task_priority
                                          : UI_DEFAULT_TASK_PRIORITY;
    int stack = config->task_stack > 0 ? config->task_stack
                                       : UI_DEFAULT_TASK_STACK;
    ui->running = true;

    BaseType_t ret = xTaskCreate(render_task, "blusys_ui", (uint32_t)stack,
                                 ui, (UBaseType_t)prio, &ui->task);
    if (ret != pdPASS) {
        free(ui->buf1);
        free(ui->buf2);
        heap_caps_free(ui->flush_buf);
        lv_display_delete(ui->disp);
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }

    s_active_handle = ui;
    *out_ui = ui;
    return BLUSYS_OK;
}

blusys_err_t blusys_ui_close(blusys_ui_t *ui)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Signal render task to stop and wait for it to exit */
    ui->running = false;
    xSemaphoreTake(ui->done_sem, portMAX_DELAY);

    lv_lock();
    lv_display_delete(ui->disp);
    lv_unlock();

    free(ui->buf1);
    free(ui->buf2);
    heap_caps_free(ui->flush_buf);
    lv_deinit();

    vSemaphoreDelete(ui->done_sem);
    s_active_handle = NULL;
    free(ui);
    return BLUSYS_OK;
}

blusys_err_t blusys_ui_lock(blusys_ui_t *ui)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    lv_lock();
    return BLUSYS_OK;
}

blusys_err_t blusys_ui_unlock(blusys_ui_t *ui)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    lv_unlock();
    return BLUSYS_OK;
}

#else /* !BLUSYS_HAS_LVGL */

blusys_err_t blusys_ui_open(const blusys_ui_config_t *config,
                            blusys_ui_t **out_ui)
{
    (void)config;
    (void)out_ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ui_close(blusys_ui_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ui_lock(blusys_ui_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ui_unlock(blusys_ui_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* BLUSYS_HAS_LVGL */
