#include "blusys/drivers/display.h"

#include <stdlib.h>
#include <string.h>

#if defined(BLUSYS_HAS_LVGL)

#include "lvgl.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "blusys/framework/observe/counter.h"
#include "blusys/framework/observe/log.h"
#include "blusys/hal/internal/esp_err_shim.h"

#define DISPLAY_DOMAIN err_domain_driver_display

typedef void (*display_flush_fn_t)(blusys_display_t *ui, const lv_area_t *area, uint8_t *px_map);

static blusys_err_t display_oom(const char *site, size_t bytes)
{
    blusys_counter_inc(DISPLAY_DOMAIN, 1);
    BLUSYS_LOG(BLUSYS_LOG_ERROR, DISPLAY_DOMAIN,
               "op=open alloc-failed site=%s bytes=%zu", site, bytes);
    return BLUSYS_ERR(DISPLAY_DOMAIN, BLUSYS_ERR_CODE(BLUSYS_ERR_NO_MEM));
}

static bool ui_wants_rgb565_spi_byte_swap(blusys_display_panel_kind_t kind)
{
    return kind == BLUSYS_DISPLAY_PANEL_KIND_RGB565;
}

#define UI_DEFAULT_TASK_PRIORITY  5
#define UI_DEFAULT_TASK_STACK     16384
#define UI_DEFAULT_BUF_LINES      20
#define UI_FLUSH_BAND_LINES       40

static blusys_display_t *s_active_handle;

struct blusys_display {
    blusys_lcd_t           *lcd;
    lv_display_t           *disp;
    void                   *buf1;
    void                   *buf2;
    void                   *flush_buf;
    size_t                  flush_buf_size;
    TaskHandle_t            task;
    SemaphoreHandle_t       done_sem;
    volatile bool           running;
    blusys_display_panel_kind_t  panel_kind;
    display_flush_fn_t      flush_fn;
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
/* MONO_PAGE flush helpers — RGB565 → SSD1306 page format             */
/* ------------------------------------------------------------------ */
/* Threshold each RGB565 pixel to 1 bit, pack into SSD1306 page format:
 * each byte = 8 vertical pixels, D0 (LSB) at top, 4 pages × width bytes
 * for a 128×32 panel.  flush_buf is sized width*height/8 bytes. */
static void flush_mono_page_pack(blusys_display_t *ui, const lv_area_t *area,
                                 const uint8_t *px_map)
{
    const uint16_t *src = (const uint16_t *)px_map;
    const int32_t   x1  = area->x1, y1 = area->y1;
    const int32_t   x2  = area->x2, y2 = area->y2;
    const int32_t   w   = x2 - x1 + 1;

    memset(ui->flush_buf, 0, ui->flush_buf_size);
    uint8_t *dst = (uint8_t *)ui->flush_buf;
    for (int32_t y = y1; y <= y2; ++y) {
        const int32_t   page = y / 8;
        const uint8_t   bit  = (uint8_t)(1u << (y % 8));
        const uint16_t *row  = src + ((size_t)(y - y1) * (size_t)w);
        for (int32_t x = x1; x <= x2; ++x) {
            if (row[x - x1] != 0u)
                dst[(size_t)page * (size_t)ui->panel_width + (size_t)x] |= bit;
        }
    }
}

static void flush_mono_page(blusys_display_t *ui, const lv_area_t *area,
                            uint8_t *px_map)
{
    flush_mono_page_pack(ui, area, (const uint8_t *)px_map);
    blusys_err_t err = blusys_lcd_draw_bitmap(ui->lcd,
                           0, 0, (int32_t)ui->panel_width, (int32_t)ui->panel_height,
                           ui->flush_buf);
    if (err != BLUSYS_OK)
        BLUSYS_LOG(BLUSYS_LOG_ERROR, DISPLAY_DOMAIN,
                   "lcd_draw_bitmap failed: %d (mono_page)", (int)err);
}

static void flush_rgb565_band(blusys_display_t *ui, const lv_area_t *area,
                              const uint8_t *px_map,
                              uint32_t area_w, uint32_t area_h,
                              uint32_t row_bytes, uint32_t src_stride);
static void flush_rgb565_row(blusys_display_t *ui, const lv_area_t *area,
                             uint8_t *px_map,
                             uint32_t area_w, uint32_t area_h,
                             uint32_t row_bytes, uint32_t src_stride);

/* RGB565 strategy — copies pixel rows into DMA scratch, byte-swaps
 * in place if required, then sends each band to the LCD. */
static void flush_rgb565(blusys_display_t *ui, const lv_area_t *area,
                         uint8_t *px_map)
{
    lv_color_format_t cf = lv_display_get_color_format(ui->disp);
    uint32_t area_w      = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t area_h      = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t row_bytes   = area_w * lv_color_format_get_size(cf);
    uint32_t src_stride  = lv_draw_buf_width_to_stride(area_w, cf);

    if (ui->flush_buf != NULL && ui->flush_buf_size >= row_bytes) {
        flush_rgb565_band(ui, area, (const uint8_t *)px_map, area_w, area_h,
                          row_bytes, src_stride);
    } else {
        flush_rgb565_row(ui, area, px_map, area_w, area_h, row_bytes, src_stride);
    }
}

/* RGB565 band flush — copies pixel rows into DMA scratch, byte-swaps
 * in place if required, then sends each band to the LCD. */
static void flush_rgb565_band(blusys_display_t *ui, const lv_area_t *area,
                              const uint8_t *px_map,
                              uint32_t area_w, uint32_t area_h,
                              uint32_t row_bytes, uint32_t src_stride)
{
    uint32_t max_rows = ui->flush_buf_size / row_bytes;
    for (uint32_t row = 0; row < area_h; row += max_rows) {
        uint32_t band = area_h - row;
        if (band > max_rows) band = max_rows;
        if (src_stride == row_bytes) {
            memcpy(ui->flush_buf, px_map + (size_t)row * src_stride,
                   (size_t)band * row_bytes);
        } else {
            for (uint32_t r = 0; r < band; r++)
                memcpy((uint8_t *)ui->flush_buf + (size_t)r * row_bytes,
                       px_map + (size_t)(row + r) * src_stride, row_bytes);
        }
        if (ui_wants_rgb565_spi_byte_swap(ui->panel_kind))
            lv_draw_sw_rgb565_swap(ui->flush_buf, area_w * band);
        blusys_err_t err = blusys_lcd_draw_bitmap(ui->lcd,
                               area->x1, area->y1 + (int32_t)row,
                               area->x2 + 1, area->y1 + (int32_t)(row + band),
                               ui->flush_buf);
        if (err != BLUSYS_OK)
            BLUSYS_LOG(BLUSYS_LOG_ERROR, DISPLAY_DOMAIN,
                       "lcd_draw_bitmap failed: %d (band row=%lu)",
                       (int)err, (unsigned long)row);
    }
}

/* RGB565 row fallback — used when DMA scratch is absent or undersized.
 * In-place byte-swap is reversed after each DMA call so LVGL's draw
 * buffer is left unmodified (composited as background by transparent widgets). */
static void flush_rgb565_row(blusys_display_t *ui, const lv_area_t *area,
                             uint8_t *px_map,
                             uint32_t area_w, uint32_t area_h,
                             uint32_t row_bytes, uint32_t src_stride)
{
    BLUSYS_LOG(BLUSYS_LOG_WARN, DISPLAY_DOMAIN,
               "flush: DMA scratch too small (have %u need >= %u); row fallback",
               (unsigned)ui->flush_buf_size, (unsigned)row_bytes);
    for (uint32_t row = 0; row < area_h; row++) {
        uint8_t *row_ptr = px_map + (size_t)row * src_stride;
        if (ui_wants_rgb565_spi_byte_swap(ui->panel_kind))
            lv_draw_sw_rgb565_swap(row_ptr, area_w);
        blusys_err_t err = blusys_lcd_draw_bitmap(ui->lcd,
                               area->x1, area->y1 + (int32_t)row,
                               area->x2 + 1, area->y1 + (int32_t)row + 1,
                               row_ptr);
        if (ui_wants_rgb565_spi_byte_swap(ui->panel_kind))
            lv_draw_sw_rgb565_swap(row_ptr, area_w);
        if (err != BLUSYS_OK)
            BLUSYS_LOG(BLUSYS_LOG_ERROR, DISPLAY_DOMAIN,
                       "lcd_draw_bitmap failed: %d (fallback row=%lu)",
                       (int)err, (unsigned long)row);
    }
}

/* ------------------------------------------------------------------ */
/* LVGL flush callback — dispatches to the selected strategy helper    */
/* ------------------------------------------------------------------ */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    blusys_display_t *ui = lv_display_get_user_data(disp);

    if (ui->flush_fn != NULL) {
        ui->flush_fn(ui, area, px_map);
    }
    lv_display_flush_ready(disp);
}

/* ------------------------------------------------------------------ */
/* Render task — calls lv_timer_handler() in a loop                    */
/* ------------------------------------------------------------------ */
static void render_task(void *arg)
{
    blusys_display_t *ui = arg;

    while (ui->running) {
        uint32_t ms = lv_timer_handler();
        if (ms > 33) {
            ms = 33;   /* cap at ~30 FPS so dirty frames aren't held up */
        }
        if (ms < 1) {
            ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(ms));
    }

    xSemaphoreGive(ui->done_sem);
    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */
blusys_err_t blusys_display_open(const blusys_display_config_t *config,
                            blusys_display_t **out_ui)
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

    blusys_display_t *ui = calloc(1, sizeof(*ui));
    if (ui == NULL) {
        return display_oom("handle", sizeof(*ui));
    }
    ui->lcd          = config->lcd;
    ui->panel_kind   = config->panel_kind;
    ui->panel_width  = width;
    ui->panel_height = height;

    ui->done_sem = xSemaphoreCreateBinary();
    if (ui->done_sem == NULL) {
        blusys_err_t oom = display_oom("done_sem", 0);
        free(ui);
        return oom;
    }

    /* LVGL core init */
    lv_init();
    lv_tick_set_cb(tick_get_cb);

    /* Create display */
    ui->disp = lv_display_create((int32_t)width, (int32_t)height);
    if (ui->disp == NULL) {
        blusys_err_t oom = display_oom("lv_display", 0);
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return oom;
    }
    lv_display_set_color_format(ui->disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(ui->disp, ui);
    lv_display_set_flush_cb(ui->disp, flush_cb);
    ui->flush_fn = (ui->panel_kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE)
                       ? flush_mono_page
                       : flush_rgb565;

    /* MONO_PAGE forces full-refresh: page buffer is rebuilt whole on each flush. */
    bool full_refresh = config->full_refresh ||
                        (ui->panel_kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE);
    uint32_t buf_lines = config->buf_lines > 0
                         ? config->buf_lines
                         : UI_DEFAULT_BUF_LINES;
    lv_color_format_t cf = lv_display_get_color_format(ui->disp);
    uint32_t stride    = lv_draw_buf_width_to_stride(width, cf);
    uint32_t buf_bytes = stride * (full_refresh ? height : buf_lines);
    uint32_t flush_band_lines = (buf_lines > UI_FLUSH_BAND_LINES)
                                ? buf_lines
                                : UI_FLUSH_BAND_LINES;
    if (flush_band_lines > height) {
        flush_band_lines = height;
    }
    uint32_t flush_buf_bytes;
    if (ui->panel_kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE) {
        flush_buf_bytes = width * height / 8u;
    } else {
        flush_buf_bytes = stride * flush_band_lines;
    }

    ui->buf1 = malloc(buf_bytes);
    if (!full_refresh) {
        ui->buf2 = malloc(buf_bytes);
    }
    ui->flush_buf = heap_caps_malloc(flush_buf_bytes,
                                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    const char *oom_site = NULL;
    size_t       oom_bytes = 0;
    if (ui->buf1 == NULL) {
        oom_site = "buf1";          oom_bytes = buf_bytes;
    } else if (!full_refresh && ui->buf2 == NULL) {
        oom_site = "buf2";          oom_bytes = buf_bytes;
    } else if (ui->flush_buf == NULL) {
        oom_site = "flush_buf_dma"; oom_bytes = flush_buf_bytes;
    }
    if (oom_site != NULL) {
        blusys_err_t err = display_oom(oom_site, oom_bytes);
        free(ui->buf1);
        free(ui->buf2);
        heap_caps_free(ui->flush_buf);
        lv_display_delete(ui->disp);
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return err;
    }
    ui->flush_buf_size = flush_buf_bytes;
    lv_display_set_buffers(ui->disp, ui->buf1, ui->buf2,
                           buf_bytes,
                           full_refresh ? LV_DISPLAY_RENDER_MODE_FULL
                                        : LV_DISPLAY_RENDER_MODE_PARTIAL);

    const char *panel_name =
        ui->panel_kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE        ? "mono_page" :
        ui->panel_kind == BLUSYS_DISPLAY_PANEL_KIND_RGB565_NATIVE     ? "rgb565_native" : "rgb565";
    BLUSYS_LOG(BLUSYS_LOG_INFO, DISPLAY_DOMAIN,
               "open: %lux%lu buf_lines=%lu buf_bytes=%lu scratch=%lu full_refresh=%d panel=%s",
               (unsigned long)width, (unsigned long)height, (unsigned long)buf_lines,
               (unsigned long)buf_bytes, (unsigned long)flush_buf_bytes,
               full_refresh ? 1 : 0, panel_name);

    /* Start render task */
    int prio  = config->task_priority > 0 ? config->task_priority
                                          : UI_DEFAULT_TASK_PRIORITY;
    int stack = config->task_stack > 0 ? config->task_stack
                                       : UI_DEFAULT_TASK_STACK;
    ui->running = true;

    BaseType_t ret = xTaskCreate(render_task, "blusys_display", (uint32_t)stack,
                                 ui, (UBaseType_t)prio, &ui->task);
    if (ret != pdPASS) {
        blusys_err_t err = display_oom("render_task", (size_t)stack);
        free(ui->buf1);
        free(ui->buf2);
        heap_caps_free(ui->flush_buf);
        lv_display_delete(ui->disp);
        lv_deinit();
        vSemaphoreDelete(ui->done_sem);
        free(ui);
        return err;
    }

    s_active_handle = ui;
    *out_ui = ui;
    return BLUSYS_OK;
}

blusys_err_t blusys_display_close(blusys_display_t *ui)
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

blusys_err_t blusys_display_lock(blusys_display_t *ui)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    lv_lock();
    return BLUSYS_OK;
}

blusys_err_t blusys_display_unlock(blusys_display_t *ui)
{
    if (ui == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    lv_unlock();
    return BLUSYS_OK;
}

void blusys_display_invalidation_begin(blusys_display_t *ui)
{
    if (ui == NULL || ui->disp == NULL) {
        return;
    }
    lv_display_enable_invalidation(ui->disp, false);
}

void blusys_display_invalidation_end(blusys_display_t *ui)
{
    if (ui == NULL || ui->disp == NULL) {
        return;
    }
    lv_display_enable_invalidation(ui->disp, true);
    lv_obj_t *scr = lv_display_get_screen_active(ui->disp);
    if (scr != NULL) {
        lv_obj_invalidate(scr);
    }
}

#else /* !BLUSYS_HAS_LVGL */

blusys_err_t blusys_display_open(const blusys_display_config_t *config,
                            blusys_display_t **out_ui)
{
    (void)config;
    (void)out_ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_display_close(blusys_display_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_display_lock(blusys_display_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_display_unlock(blusys_display_t *ui)
{
    (void)ui;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

void blusys_display_invalidation_begin(blusys_display_t *ui)
{
    (void)ui;
}

void blusys_display_invalidation_end(blusys_display_t *ui)
{
    (void)ui;
}

#endif /* BLUSYS_HAS_LVGL */
