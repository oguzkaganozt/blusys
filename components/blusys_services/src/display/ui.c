#include "blusys/display/ui.h"

#include <stdlib.h>

#if defined(BLUSYS_HAS_LVGL)

#include "lvgl.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "blusys/internal/blusys_esp_err.h"

#define UI_DEFAULT_TASK_PRIORITY  5
#define UI_DEFAULT_TASK_STACK     16384
#define UI_DEFAULT_BUF_LINES      20

static blusys_ui_t *s_active_handle;

struct blusys_ui {
    blusys_lcd_t      *lcd;
    lv_display_t      *disp;
    void              *buf1;
    void              *buf2;
    TaskHandle_t       task;
    SemaphoreHandle_t  done_sem;
    volatile bool      running;
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
    lv_draw_buf_t *draw_buf = lv_display_get_buf_active(disp);
    uint32_t area_width = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t area_height = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t pixel_size = lv_color_format_get_size(lv_display_get_color_format(disp));
    uint32_t packed_row_bytes = area_width * pixel_size;
    uint32_t row_stride = packed_row_bytes;

    if ((draw_buf != NULL) && (draw_buf->header.stride != 0u)) {
        row_stride = draw_buf->header.stride;
    }

    for (uint32_t row = 0; row < area_height; row++) {
        uint8_t *row_ptr = px_map + ((size_t)row * row_stride);

        lv_draw_sw_rgb565_swap(row_ptr, area_width);

        blusys_lcd_draw_bitmap(ui->lcd,
                               area->x1,
                               area->y1 + (int32_t)row,
                               area->x2 + 1,
                               area->y1 + (int32_t)row + 1,
                               row_ptr);
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
    uint32_t buf_lines = config->buf_lines > 0
                         ? config->buf_lines
                         : UI_DEFAULT_BUF_LINES;
    lv_color_format_t cf = lv_display_get_color_format(ui->disp);
    uint32_t stride = lv_draw_buf_width_to_stride(width, cf);
    uint32_t buf_bytes = stride * buf_lines;

    ui->buf1 = malloc(buf_bytes);
    ui->buf2 = malloc(buf_bytes);
    if (ui->buf1 == NULL || ui->buf2 == NULL) {
        free(ui->buf1);
        free(ui->buf2);
        lv_display_delete(ui->disp);
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return BLUSYS_ERR_NO_MEM;
    }
    /* Keep two partial render buffers available for LVGL's flush pipeline. */
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
        free(ui->buf2);
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
