#include "blusys/display/ui.h"

#include <stdlib.h>

#if defined(BLUSYS_HAS_LVGL)

#include "lvgl.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/internal/blusys_esp_err.h"

#define UI_DEFAULT_TASK_PRIORITY  5
#define UI_DEFAULT_TASK_STACK     8192
#define UI_DEFAULT_BUF_LINES      20

struct blusys_ui {
    blusys_lcd_t   *lcd;
    lv_display_t   *disp;
    void           *buf1;
    void           *buf2;
    TaskHandle_t    task;
    volatile bool   running;
};

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
static void flush_cb(lv_display_t *disp, const lv_area_t *area,
                     uint8_t *px_map)
{
    blusys_ui_t *ui = lv_display_get_user_data(disp);

    /* SPI LCDs expect big-endian RGB565; ESP32 is little-endian — swap bytes */
    uint32_t px_count = (uint32_t)(area->x2 - area->x1 + 1) *
                        (uint32_t)(area->y2 - area->y1 + 1);
    uint16_t *px16 = (uint16_t *)px_map;
    for (uint32_t i = 0; i < px_count; i++) {
        px16[i] = (px16[i] >> 8) | (px16[i] << 8);
    }

    /* blusys_lcd_draw_bitmap queues a DMA transfer and returns before it
     * completes.  With double buffering, LVGL renders into the OTHER buffer
     * after flush_ready, so this buffer stays untouched until the next flush
     * call — which blocks (trans_queue_depth=1) until this DMA finishes. */
    blusys_lcd_draw_bitmap(ui->lcd,
                           area->x1, area->y1,
                           area->x2 + 1, area->y2 + 1,
                           px_map);

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

    /* LVGL core init */
    lv_init();
    lv_tick_set_cb(tick_get_cb);

    /* Create display */
    ui->disp = lv_display_create((int32_t)width, (int32_t)height);
    if (ui->disp == NULL) {
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }
    lv_display_set_user_data(ui->disp, ui);
    lv_display_set_flush_cb(ui->disp, flush_cb);

    /* Allocate draw buffer.
     * LVGL v9 internally computes stride = width * bytes_per_pixel (for RGB565).
     * lv_display_set_buffers() divides buf_size by stride to get buffer height.
     * We must match that: stride * buf_lines = buf_bytes. */
    uint32_t buf_lines = config->buf_size > 0
                         ? config->buf_size / width
                         : UI_DEFAULT_BUF_LINES;
    if (buf_lines < 1) {
        buf_lines = 1;
    }
    lv_color_format_t cf = lv_display_get_color_format(ui->disp);
    uint32_t stride = lv_draw_buf_width_to_stride(width, cf);
    uint32_t buf_bytes = stride * buf_lines;

    ui->buf1 = malloc(buf_bytes);
    ui->buf2 = malloc(buf_bytes);
    if (ui->buf1 == NULL || ui->buf2 == NULL) {
        free(ui->buf1);
        free(ui->buf2);
        lv_display_delete(ui->disp);
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }
    /* Double buffering: LVGL renders into one buffer while the other is
     * being DMA'd to the display, preventing data corruption. */
    lv_display_set_buffers(ui->disp, ui->buf1, ui->buf2,
                           buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

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
        lv_display_delete(ui->disp);
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }

    *out_ui = ui;
    return BLUSYS_OK;
}

blusys_err_t blusys_ui_close(blusys_ui_t *ui)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Stop render task */
    ui->running = false;
    /* Give the task time to exit */
    vTaskDelay(pdMS_TO_TICKS(100));

    lv_display_delete(ui->disp);
    free(ui->buf1);
    free(ui->buf2);
    lv_deinit();
    free(ui);
    return BLUSYS_OK;
}

blusys_err_t blusys_ui_lock(blusys_ui_t *ui, int timeout_ms)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    (void)timeout_ms;
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

blusys_err_t blusys_ui_lock(blusys_ui_t *ui, int timeout_ms)
{
    (void)ui;
    (void)timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ui_unlock(blusys_ui_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* BLUSYS_HAS_LVGL */
