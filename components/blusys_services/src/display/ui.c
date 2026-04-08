#include "blusys/display/ui.h"

#include <stdlib.h>
#include <string.h>

#if defined(BLUSYS_HAS_LVGL)

#include "sdkconfig.h"
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
#define UI_FLUSH_STRIPE_WIDTH     40

typedef enum {
    UI_FLUSH_MODE_ROW_STRIDE = 0,
    UI_FLUSH_MODE_ROW_PACKED,
    UI_FLUSH_MODE_BAND_STRIDE,
    UI_FLUSH_MODE_BAND_PACKED,
    UI_FLUSH_MODE_DIRECT_PACKED,
} ui_flush_mode_t;

static blusys_ui_t *s_active_handle;

struct blusys_ui {
    blusys_lcd_t      *lcd;
    lv_display_t      *disp;
    void              *buf1;
    void              *buf2;
    void              *flush_buf;
    size_t             flush_buf_size;
    TaskHandle_t       task;
    SemaphoreHandle_t  done_sem;
    volatile bool      running;
};

static ui_flush_mode_t ui_flush_mode_get(void)
{
#if defined(CONFIG_BLUSYS_UI_DIAG_FLUSH_ROW_PACKED)
    return UI_FLUSH_MODE_ROW_PACKED;
#elif defined(CONFIG_BLUSYS_UI_DIAG_FLUSH_BAND_STRIDE)
    return UI_FLUSH_MODE_BAND_STRIDE;
#elif defined(CONFIG_BLUSYS_UI_DIAG_FLUSH_BAND_PACKED)
    return UI_FLUSH_MODE_BAND_PACKED;
#elif defined(CONFIG_BLUSYS_UI_DIAG_FLUSH_DIRECT_PACKED)
    return UI_FLUSH_MODE_DIRECT_PACKED;
#else
    return UI_FLUSH_MODE_ROW_STRIDE;
#endif
}

static const char *ui_flush_mode_name(ui_flush_mode_t mode)
{
    switch (mode) {
    case UI_FLUSH_MODE_ROW_STRIDE:
        return "row_stride";
    case UI_FLUSH_MODE_ROW_PACKED:
        return "row_packed";
    case UI_FLUSH_MODE_BAND_STRIDE:
        return "band_stride";
    case UI_FLUSH_MODE_BAND_PACKED:
        return "band_packed";
    case UI_FLUSH_MODE_DIRECT_PACKED:
        return "direct_packed";
    default:
        return "unknown";
    }
}

static bool ui_flush_log_enabled(void)
{
#if defined(CONFIG_BLUSYS_UI_DIAG_LOG_FLUSH)
    return true;
#else
    return false;
#endif
}

static void ui_log_flush(uint32_t seq,
                         ui_flush_mode_t mode,
                         const lv_area_t *area,
                         const lv_draw_buf_t *draw_buf,
                         const void *px_map,
                         uint32_t packed_row_bytes,
                         uint32_t draw_stride,
                         uint32_t source_row_bytes,
                         uint32_t flush_buf_size,
                         uint32_t max_band_rows)
{
    if (!ui_flush_log_enabled()) {
        return;
    }

    ESP_LOGI(TAG,
             "flush[%lu] mode=%s area=(%ld,%ld)-(%ld,%ld) w=%ld h=%ld px=%p draw=%p draw_w=%u draw_h=%u draw_stride=%lu packed=%lu src=%lu scratch=%lu band_rows=%lu",
             (unsigned long)seq,
             ui_flush_mode_name(mode),
             (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2,
             (long)(area->x2 - area->x1 + 1),
             (long)(area->y2 - area->y1 + 1),
             px_map,
             (const void *)draw_buf,
             draw_buf != NULL ? draw_buf->header.w : 0u,
             draw_buf != NULL ? draw_buf->header.h : 0u,
             (unsigned long)draw_stride,
             (unsigned long)packed_row_bytes,
             (unsigned long)source_row_bytes,
             (unsigned long)flush_buf_size,
             (unsigned long)max_band_rows);

    if ((draw_buf != NULL) && (draw_stride != packed_row_bytes)) {
        ESP_LOGW(TAG,
                 "flush[%lu] draw_stride (%lu) differs from packed row bytes (%lu)",
                 (unsigned long)seq,
                 (unsigned long)draw_stride,
                 (unsigned long)packed_row_bytes);
    }
}

static void ui_log_flush_error(uint32_t seq,
                               int x_start, int y_start,
                               int x_end, int y_end,
                               blusys_err_t err)
{
    if (err == BLUSYS_OK) {
        return;
    }

    ESP_LOGE(TAG,
             "flush[%lu] draw_bitmap (%d,%d)-(%d,%d) failed: %s",
             (unsigned long)seq,
             x_start, y_start, x_end, y_end,
             blusys_err_string(err));
}

static void ui_log_rgb565_samples(uint32_t seq,
                                  const lv_area_t *area,
                                  const uint8_t *px_map,
                                  uint32_t source_row_bytes,
                                  uint32_t area_width,
                                  uint32_t area_height)
{
    static uint32_t s_top_left_sample_count;

    if (!ui_flush_log_enabled()) {
        return;
    }

    if ((area->x1 != 0) || (area->y1 != 0)) {
        return;
    }

    s_top_left_sample_count++;
    if (s_top_left_sample_count > 2u) {
        return;
    }

    uint32_t rows = area_height < 8u ? area_height : 8u;
    uint32_t cols = area_width < 8u ? area_width : 8u;

    ESP_LOGI(TAG,
             "flush[%lu] tl sample set %lu",
             (unsigned long)seq,
             (unsigned long)s_top_left_sample_count);

    for (uint32_t row = 0; row < rows; row++) {
        const uint16_t *src = (const uint16_t *)(px_map + ((size_t)row * source_row_bytes));

        switch (cols) {
        case 8:
            ESP_LOGI(TAG,
                     "flush[%lu] tl row%lu: %04x %04x %04x %04x %04x %04x %04x %04x",
                     (unsigned long)seq,
                     (unsigned long)row,
                     src[0], src[1], src[2], src[3],
                     src[4], src[5], src[6], src[7]);
            break;
        case 7:
            ESP_LOGI(TAG,
                     "flush[%lu] tl row%lu: %04x %04x %04x %04x %04x %04x %04x",
                     (unsigned long)seq,
                     (unsigned long)row,
                     src[0], src[1], src[2], src[3],
                     src[4], src[5], src[6]);
            break;
        case 6:
            ESP_LOGI(TAG,
                     "flush[%lu] tl row%lu: %04x %04x %04x %04x %04x %04x",
                     (unsigned long)seq,
                     (unsigned long)row,
                     src[0], src[1], src[2], src[3], src[4], src[5]);
            break;
        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* LVGL tick source — returns elapsed ms via esp_timer                 */
/* ------------------------------------------------------------------ */
static uint32_t tick_get_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/* ------------------------------------------------------------------ */
/* LVGL flush callback — forwards to blusys_lcd_draw_bitmap()          */
/* ------------------------------------------------------------------ */
/* Copy each LVGL flush area into our own DMA-safe scratch buffer, then send
 * it to the panel in larger packed bands. This keeps LVGL buffer lifetime and
 * stride handling separate from the SPI transfer path. */
static void flush_cb(lv_display_t *disp, const lv_area_t *area,
                     uint8_t *px_map)
{
    blusys_ui_t *ui = lv_display_get_user_data(disp);
    lv_draw_buf_t *draw_buf = lv_display_get_buf_active(disp);
    static uint32_t s_flush_seq;
    uint32_t seq = ++s_flush_seq;
    uint32_t area_width = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t area_height = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t pixel_size = lv_color_format_get_size(lv_display_get_color_format(disp));
    uint32_t packed_row_bytes = area_width * pixel_size;
    uint32_t draw_stride = packed_row_bytes;
    uint32_t max_band_rows = 0u;
    ui_flush_mode_t mode = ui_flush_mode_get();
    uint32_t source_row_bytes;

    if ((draw_buf != NULL) && (draw_buf->header.stride != 0u)) {
        draw_stride = draw_buf->header.stride;
    }
    if (draw_stride < packed_row_bytes) {
        draw_stride = packed_row_bytes;
    }
    if ((ui->flush_buf != NULL) && (ui->flush_buf_size >= packed_row_bytes)) {
        max_band_rows = (uint32_t)(ui->flush_buf_size / packed_row_bytes);
        if (max_band_rows == 0u) {
            max_band_rows = 1u;
        }
    }

    switch (mode) {
    case UI_FLUSH_MODE_ROW_PACKED:
    case UI_FLUSH_MODE_BAND_PACKED:
    case UI_FLUSH_MODE_DIRECT_PACKED:
        source_row_bytes = packed_row_bytes;
        break;
    case UI_FLUSH_MODE_ROW_STRIDE:
    case UI_FLUSH_MODE_BAND_STRIDE:
    default:
        source_row_bytes = draw_stride;
        break;
    }

    ui_log_flush(seq, mode, area, draw_buf, px_map,
                 packed_row_bytes, draw_stride, source_row_bytes,
                 (uint32_t)ui->flush_buf_size, max_band_rows);
    ui_log_rgb565_samples(seq, area, px_map, source_row_bytes,
                          area_width, area_height);

    if (mode == UI_FLUSH_MODE_DIRECT_PACKED) {
        blusys_err_t err;

        lv_draw_sw_rgb565_swap(px_map, area_width * area_height);
        err = blusys_lcd_draw_bitmap(ui->lcd,
                                     area->x1,
                                     area->y1,
                                     area->x2 + 1,
                                     area->y2 + 1,
                                     px_map);
        ui_log_flush_error(seq,
                           area->x1, area->y1,
                           area->x2 + 1, area->y2 + 1,
                           err);
        lv_display_flush_ready(disp);
        return;
    }

    if (((mode == UI_FLUSH_MODE_BAND_STRIDE) ||
         (mode == UI_FLUSH_MODE_BAND_PACKED)) &&
        (max_band_rows > 0u)) {
        for (uint32_t row = 0; row < area_height; row += max_band_rows) {
            uint32_t band_rows = area_height - row;

            if (band_rows > max_band_rows) {
                band_rows = max_band_rows;
            }

            for (uint32_t col = 0; col < area_width; col += UI_FLUSH_STRIPE_WIDTH) {
                blusys_err_t err;
                uint32_t stripe_width = area_width - col;
                uint32_t stripe_row_bytes;
                uint8_t *band_ptr = ui->flush_buf;

                if (stripe_width > UI_FLUSH_STRIPE_WIDTH) {
                    stripe_width = UI_FLUSH_STRIPE_WIDTH;
                }

                stripe_row_bytes = stripe_width * pixel_size;

                for (uint32_t band_row = 0; band_row < band_rows; band_row++) {
                    uint8_t *dst_row = band_ptr + ((size_t)band_row * stripe_row_bytes);
                    uint8_t *src_row = px_map + ((size_t)(row + band_row) * source_row_bytes) +
                                       ((size_t)col * pixel_size);

                    memcpy(dst_row, src_row, stripe_row_bytes);
                }
                lv_draw_sw_rgb565_swap(band_ptr, stripe_width * band_rows);

                err = blusys_lcd_draw_bitmap(ui->lcd,
                                             area->x1 + (int32_t)col,
                                             area->y1 + (int32_t)row,
                                             area->x1 + (int32_t)(col + stripe_width),
                                             area->y1 + (int32_t)(row + band_rows),
                                             band_ptr);
                ui_log_flush_error(seq,
                                   area->x1 + (int32_t)col,
                                   area->y1 + (int32_t)row,
                                   area->x1 + (int32_t)(col + stripe_width),
                                   area->y1 + (int32_t)(row + band_rows),
                                   err);
            }
        }
        lv_display_flush_ready(disp);
        return;
    }

    if ((mode == UI_FLUSH_MODE_BAND_STRIDE) ||
        (mode == UI_FLUSH_MODE_BAND_PACKED)) {
        ESP_LOGW(TAG,
                 "flush[%lu] selected %s but scratch buffer is unavailable; falling back to row mode",
                 (unsigned long)seq,
                 ui_flush_mode_name(mode));
    }

    for (uint32_t row = 0; row < area_height; row++) {
        blusys_err_t err;
        uint8_t *row_ptr = px_map + ((size_t)row * source_row_bytes);

        lv_draw_sw_rgb565_swap(row_ptr, area_width);

        err = blusys_lcd_draw_bitmap(ui->lcd,
                                     area->x1,
                                     area->y1 + (int32_t)row,
                                     area->x2 + 1,
                                     area->y1 + (int32_t)row + 1,
                                     row_ptr);
        ui_log_flush_error(seq,
                           area->x1,
                           area->y1 + (int32_t)row,
                           area->x2 + 1,
                           area->y1 + (int32_t)row + 1,
                           err);
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
    ui->lcd = config->lcd;

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

    /* Allocate draw buffers */
    bool full_refresh = config->full_refresh;
    uint32_t buf_lines = config->buf_lines > 0
                         ? config->buf_lines
                         : UI_DEFAULT_BUF_LINES;
    lv_color_format_t cf = lv_display_get_color_format(ui->disp);
    uint32_t stride = lv_draw_buf_width_to_stride(width, cf);
    uint32_t buf_bytes = stride * (full_refresh ? height : buf_lines);
    /* Size the scratch buffer for at least one typical LVGL chunk and cap it
     * to the panel height so full-screen invalidations can still be streamed
     * in a few larger LCD transactions. */
    uint32_t flush_band_lines = (buf_lines > UI_FLUSH_BAND_LINES)
                                ? buf_lines
                                : UI_FLUSH_BAND_LINES;
    if (flush_band_lines > height) {
        flush_band_lines = height;
    }
    uint32_t flush_buf_bytes = stride * flush_band_lines;

    ui->buf1 = malloc(buf_bytes);
    ui->buf2 = full_refresh ? NULL : malloc(buf_bytes);
    ui->flush_buf = heap_caps_malloc(flush_buf_bytes,
                                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (ui->buf1 == NULL ||
        (!full_refresh && ui->buf2 == NULL) ||
        (ui->flush_buf == NULL)) {
        free(ui->buf1);
        free(ui->buf2);
        if (ui->flush_buf != NULL) {
            heap_caps_free(ui->flush_buf);
        }
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
             "ui open: %lux%lu buf_lines=%lu buf_bytes=%lu scratch=%lu full_refresh=%d flush_mode=%s",
             (unsigned long)width,
             (unsigned long)height,
             (unsigned long)buf_lines,
             (unsigned long)buf_bytes,
             (unsigned long)flush_buf_bytes,
             full_refresh ? 1 : 0,
             ui_flush_mode_name(ui_flush_mode_get()));

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
