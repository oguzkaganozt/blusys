#include "blusys/drivers/display/lcd.h"

#include <stdbool.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_nt35510.h"

struct blusys_lcd {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t    panel_handle;
    blusys_lock_t             lock;
    SemaphoreHandle_t         color_done_sem;
    int                       bl_pin;
    blusys_lcd_driver_t       driver;
    spi_host_device_t         spi_host;
    i2c_master_bus_handle_t   i2c_bus;
    bool                      spi_bus_owned;
    bool                      wait_color_trans_done;
    uint32_t                  width;
    uint32_t                  height;
};

typedef struct {
    esp_lcd_panel_t           base;
    esp_lcd_panel_io_handle_t io_handle;
    int                       reset_gpio_num;
    bool                      reset_level;
    int                       x_gap;
    int                       y_gap;
    uint8_t                   fb_bits_per_pixel;
    uint8_t                   madctl_val;
    uint8_t                   colmod_val;
} blusys_st7735_panel_t;

typedef struct {
    uint8_t        cmd;
    uint8_t        len;
    const uint8_t *data;
} blusys_lcd_init_cmd_t;

static bool IRAM_ATTR blusys_lcd_spi_color_done_cb(esp_lcd_panel_io_handle_t panel_io,
                                                   esp_lcd_panel_io_event_data_t *edata,
                                                   void *user_ctx)
{
    blusys_lcd_t *lcd = user_ctx;
    BaseType_t higher_priority_woken = pdFALSE;

    (void)panel_io;
    (void)edata;

    if ((lcd != NULL) && (lcd->color_done_sem != NULL)) {
        xSemaphoreGiveFromISR(lcd->color_done_sem, &higher_priority_woken);
    }

    return higher_priority_woken == pdTRUE;
}

static blusys_st7735_panel_t *blusys_st7735_from_panel(esp_lcd_panel_t *panel)
{
    return (blusys_st7735_panel_t *)panel;
}

static int blusys_st7735_effective_x_gap(const blusys_st7735_panel_t *panel)
{
    return panel->x_gap;
}

static int blusys_st7735_effective_y_gap(const blusys_st7735_panel_t *panel)
{
    return panel->y_gap;
}

static esp_err_t blusys_st7735_panel_del(esp_lcd_panel_t *panel)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);

    if (st7735->reset_gpio_num >= 0) {
        gpio_reset_pin((gpio_num_t)st7735->reset_gpio_num);
    }

    free(st7735);
    return ESP_OK;
}

static esp_err_t blusys_st7735_panel_reset(esp_lcd_panel_t *panel)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;

    if (st7735->reset_gpio_num >= 0) {
        gpio_set_level((gpio_num_t)st7735->reset_gpio_num, st7735->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)st7735->reset_gpio_num, !st7735->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else {
        esp_err_t esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_SWRESET,
                                                      NULL, 0);
        if (esp_err != ESP_OK) {
            return esp_err;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return ESP_OK;
}

static esp_err_t blusys_st7735_panel_init(esp_lcd_panel_t *panel)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;
    static const uint8_t cmd_b1[] = {0x01, 0x2C, 0x2D};
    static const uint8_t cmd_b2[] = {0x01, 0x2C, 0x2D};
    static const uint8_t cmd_b3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    static const uint8_t cmd_b4[] = {0x07};
    static const uint8_t cmd_c0[] = {0xA2, 0x02, 0x84};
    static const uint8_t cmd_c1[] = {0xC5};
    static const uint8_t cmd_c2[] = {0x0A, 0x00};
    static const uint8_t cmd_c3[] = {0x8A, 0x2A};
    static const uint8_t cmd_c4[] = {0x8A, 0xEE};
    static const uint8_t cmd_c5[] = {0x0E};
    static const uint8_t cmd_e0[] = {
        0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
        0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    };
    static const uint8_t cmd_e1[] = {
        0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    };
    static const blusys_lcd_init_cmd_t init_cmds[] = {
        {0xB1, sizeof(cmd_b1), cmd_b1},
        {0xB2, sizeof(cmd_b2), cmd_b2},
        {0xB3, sizeof(cmd_b3), cmd_b3},
        {0xB4, sizeof(cmd_b4), cmd_b4},
        {0xC0, sizeof(cmd_c0), cmd_c0},
        {0xC1, sizeof(cmd_c1), cmd_c1},
        {0xC2, sizeof(cmd_c2), cmd_c2},
        {0xC3, sizeof(cmd_c3), cmd_c3},
        {0xC4, sizeof(cmd_c4), cmd_c4},
        {0xC5, sizeof(cmd_c5), cmd_c5},
    };
    esp_err_t esp_err;

    esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_SLPOUT, NULL, 0);
    if (esp_err != ESP_OK) {
        return esp_err;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    for (size_t i = 0; i < (sizeof(init_cmds) / sizeof(init_cmds[0])); i++) {
        esp_err = esp_lcd_panel_io_tx_param(io_handle,
                                            init_cmds[i].cmd,
                                            init_cmds[i].data,
                                            init_cmds[i].len);
        if (esp_err != ESP_OK) {
            return esp_err;
        }
    }

    esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_INVOFF, NULL, 0);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_MADCTL,
                                        &st7735->madctl_val, 1);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_COLMOD,
                                        &st7735->colmod_val, 1);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    esp_err = esp_lcd_panel_io_tx_param(io_handle, 0xE0, cmd_e0, sizeof(cmd_e0));
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    esp_err = esp_lcd_panel_io_tx_param(io_handle, 0xE1, cmd_e1, sizeof(cmd_e1));
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_NORON, NULL, 0);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    return ESP_OK;
}

static esp_err_t blusys_st7735_panel_draw_bitmap(esp_lcd_panel_t *panel,
                                                 int x_start, int y_start,
                                                 int x_end, int y_end,
                                                 const void *color_data)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;
    esp_err_t esp_err;
    int x_gap = blusys_st7735_effective_x_gap(st7735);
    int y_gap = blusys_st7735_effective_y_gap(st7735);
    size_t row_bytes;
    size_t max_rows_per_tx;
    const uint8_t *src = color_data;

    x_start += x_gap;
    x_end += x_gap;
    y_start += y_gap;
    y_end += y_gap;

    row_bytes = (size_t)(x_end - x_start) * st7735->fb_bits_per_pixel / 8u;
    max_rows_per_tx = row_bytes > 0u ? (size_t)(SPI_MAX_DMA_LEN / row_bytes) : 0u;
    if (max_rows_per_tx == 0u) {
        max_rows_per_tx = 1u;
    }

    /* Large ST7735 bitmap writes on esp32c3 become unreliable once the SPI
     * master has to chain multiple DMA descriptors for a single RAMWR burst.
     * Split them into row-aligned chunks that stay within one SPI DMA block,
     * which keeps LVGL's fast band flush path correct while remaining much
     * faster than one-row writes. */
    for (int row = 0; row < (y_end - y_start); row += (int)max_rows_per_tx) {
        int chunk_rows = (y_end - y_start) - row;
        size_t len;

        if (chunk_rows > (int)max_rows_per_tx) {
            chunk_rows = (int)max_rows_per_tx;
        }

        /* Re-assert the pixel format on every draw. This looks redundant because
         * COLMOD is also set in panel_init, but the earlier investigation showed
         * that without it the ST7735 will occasionally drop back to a different
         * pixel format mid-session, producing sheared/corrupted output when
         * several bitmap writes arrive back-to-back (e.g. from the LVGL flush
         * callback). Do not remove. */
        esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_COLMOD,
                                            &st7735->colmod_val, 1);
        if (esp_err != ESP_OK) {
            return esp_err;
        }

        esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_CASET, (uint8_t[]) {
            (x_start >> 8) & 0xFF,
            x_start & 0xFF,
            ((x_end - 1) >> 8) & 0xFF,
            (x_end - 1) & 0xFF,
        }, 4);
        if (esp_err != ESP_OK) {
            return esp_err;
        }

        esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_RASET, (uint8_t[]) {
            ((y_start + row) >> 8) & 0xFF,
            (y_start + row) & 0xFF,
            ((y_start + row + chunk_rows - 1) >> 8) & 0xFF,
            (y_start + row + chunk_rows - 1) & 0xFF,
        }, 4);
        if (esp_err != ESP_OK) {
            return esp_err;
        }

        len = row_bytes * (size_t)chunk_rows;

        esp_err = esp_lcd_panel_io_tx_color(io_handle, LCD_CMD_RAMWR, src, len);
        if (esp_err != ESP_OK) {
            return esp_err;
        }

        src += len;
    }

    /* Final sync transaction: acquiring the SPI bus here waits for the last
     * async RAMWR DMA to finish, ensuring all color_done callbacks have fired
     * before this function returns.  Without this, callers that reuse the
     * source buffer (e.g. the LVGL flush scratch buffer) on the very next
     * call can corrupt the tail of a multi-chunk transfer. */
    esp_err = esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_COLMOD,
                                        &st7735->colmod_val, 1);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    return ESP_OK;
}

static esp_err_t blusys_st7735_panel_invert_color(esp_lcd_panel_t *panel,
                                                  bool invert_color_data)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;
    int command = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;

    return esp_lcd_panel_io_tx_param(io_handle, command, NULL, 0);
}

static esp_err_t blusys_st7735_panel_mirror(esp_lcd_panel_t *panel,
                                            bool mirror_x, bool mirror_y)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;

    if (mirror_x) {
        st7735->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        st7735->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        st7735->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        st7735->madctl_val &= ~LCD_CMD_MY_BIT;
    }

    return esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_MADCTL,
                                     &st7735->madctl_val, 1);
}

static esp_err_t blusys_st7735_panel_swap_xy(esp_lcd_panel_t *panel,
                                             bool swap_axes)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;

    if (swap_axes) {
        st7735->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        st7735->madctl_val &= ~LCD_CMD_MV_BIT;
    }

    return esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_MADCTL,
                                     &st7735->madctl_val, 1);
}

static esp_err_t blusys_st7735_panel_set_gap(esp_lcd_panel_t *panel,
                                             int x_gap, int y_gap)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);

    st7735->x_gap = x_gap;
    st7735->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t blusys_st7735_panel_disp_on_off(esp_lcd_panel_t *panel,
                                                 bool on_off)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;
    int command = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;

    return esp_lcd_panel_io_tx_param(io_handle, command, NULL, 0);
}

static esp_err_t blusys_st7735_panel_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    blusys_st7735_panel_t *st7735 = blusys_st7735_from_panel(panel);
    esp_lcd_panel_io_handle_t io_handle = st7735->io_handle;
    int command = sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT;
    esp_err_t esp_err;

    esp_err = esp_lcd_panel_io_tx_param(io_handle, command, NULL, 0);
    if (esp_err != ESP_OK) {
        return esp_err;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

static esp_err_t blusys_lcd_new_panel_st7735(const esp_lcd_panel_io_handle_t io_handle,
                                             const esp_lcd_panel_dev_config_t *panel_cfg,
                                             esp_lcd_panel_handle_t *ret_panel)
{
    blusys_st7735_panel_t *st7735;
    gpio_config_t reset_cfg = {0};
    esp_err_t esp_err;
    bool reset_gpio_configured = false;

    if ((io_handle == NULL) || (panel_cfg == NULL) || (ret_panel == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    st7735 = calloc(1, sizeof(*st7735));
    if (st7735 == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (panel_cfg->reset_gpio_num >= 0) {
        reset_cfg.mode = GPIO_MODE_OUTPUT;
        reset_cfg.pin_bit_mask = 1ULL << (uint32_t)panel_cfg->reset_gpio_num;
        esp_err = gpio_config(&reset_cfg);
        if (esp_err != ESP_OK) {
            free(st7735);
            return esp_err;
        }
        reset_gpio_configured = true;
    }

    switch (panel_cfg->rgb_ele_order) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        st7735->madctl_val = 0;
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        st7735->madctl_val = LCD_CMD_BGR_BIT;
        break;
    default:
        if (reset_gpio_configured) {
            gpio_reset_pin((gpio_num_t)panel_cfg->reset_gpio_num);
        }
        free(st7735);
        return ESP_ERR_NOT_SUPPORTED;
    }

    switch (panel_cfg->bits_per_pixel) {
    case 16:
        st7735->colmod_val = 0x05;
        st7735->fb_bits_per_pixel = 16;
        break;
    case 18:
        st7735->colmod_val = 0x06;
        st7735->fb_bits_per_pixel = 24;
        break;
    default:
        if (reset_gpio_configured) {
            gpio_reset_pin((gpio_num_t)panel_cfg->reset_gpio_num);
        }
        free(st7735);
        return ESP_ERR_NOT_SUPPORTED;
    }

    st7735->io_handle = io_handle;
    st7735->reset_gpio_num = panel_cfg->reset_gpio_num;
    st7735->reset_level = panel_cfg->flags.reset_active_high;
    st7735->base.del = blusys_st7735_panel_del;
    st7735->base.reset = blusys_st7735_panel_reset;
    st7735->base.init = blusys_st7735_panel_init;
    st7735->base.draw_bitmap = blusys_st7735_panel_draw_bitmap;
    st7735->base.invert_color = blusys_st7735_panel_invert_color;
    st7735->base.mirror = blusys_st7735_panel_mirror;
    st7735->base.swap_xy = blusys_st7735_panel_swap_xy;
    st7735->base.set_gap = blusys_st7735_panel_set_gap;
    st7735->base.disp_on_off = blusys_st7735_panel_disp_on_off;
    st7735->base.disp_sleep = blusys_st7735_panel_sleep;
    *ret_panel = &st7735->base;
    return ESP_OK;
}

static bool blusys_lcd_is_valid_spi_bus(int bus)
{
    if (bus == 0) {
        return true;
    }

#if SOC_SPI_PERIPH_NUM > 2
    return bus == 1;
#else
    return false;
#endif
}

static spi_host_device_t blusys_lcd_spi_bus_to_host(int bus)
{
    if (bus == 0) {
        return SPI2_HOST;
    }

#if SOC_SPI_PERIPH_NUM > 2
    return SPI3_HOST;
#else
    return SPI2_HOST;
#endif
}

static void blusys_lcd_teardown(blusys_lcd_t *lcd)
{
    if (lcd->bl_pin != -1) {
        gpio_set_level((gpio_num_t)lcd->bl_pin, 0);
        gpio_reset_pin((gpio_num_t)lcd->bl_pin);
    }
    if (lcd->panel_handle != NULL) {
        esp_lcd_panel_del(lcd->panel_handle);
    }
    if (lcd->io_handle != NULL) {
        esp_lcd_panel_io_del(lcd->io_handle);
    }
    if (lcd->color_done_sem != NULL) {
        vSemaphoreDelete(lcd->color_done_sem);
    }
    if (lcd->i2c_bus != NULL) {
        i2c_del_master_bus(lcd->i2c_bus);
    } else if (lcd->spi_bus_owned) {
        spi_bus_free(lcd->spi_host);
    }
    blusys_lock_deinit(&lcd->lock);
    free(lcd);
}

static blusys_err_t blusys_lcd_open_spi(const blusys_lcd_config_t *config,
                                        blusys_lcd_t *lcd)
{
    esp_err_t esp_err;
    blusys_err_t err;
    spi_host_device_t host = blusys_lcd_spi_bus_to_host(config->spi.bus);
    /* Size the SPI bus so a full-frame transfer fits for small panels (e.g.
     * ST7735 160x128 = 40 KB) while still keeping at least the legacy 80-line
     * budget for taller displays. This is what lets the LVGL flush callback
     * push a whole dirty area in a single DMA transaction instead of slicing
     * it into per-row writes. */
    uint32_t xfer_lines = config->height > 80u ? config->height : 80u;
    uint32_t xfer_bytes = (config->width * xfer_lines * config->bits_per_pixel + 7u) / 8u;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num   = config->spi.mosi_pin,
        .miso_io_num   = -1,
        .sclk_io_num   = config->spi.sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)xfer_bytes,
    };

    esp_err = spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    lcd->spi_host = host;
    lcd->spi_bus_owned = true;
    lcd->color_done_sem = xSemaphoreCreateBinary();
    if (lcd->color_done_sem == NULL) {
        spi_bus_free(host);
        lcd->spi_bus_owned = false;
        return BLUSYS_ERR_NO_MEM;
    }
    lcd->wait_color_trans_done = true;

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = config->spi.dc_pin,
        .cs_gpio_num       = config->spi.cs_pin,
        .pclk_hz           = config->spi.pclk_hz,
        .on_color_trans_done = blusys_lcd_spi_color_done_cb,
        .user_ctx          = lcd,
        .lcd_cmd_bits      = (config->driver == BLUSYS_LCD_DRIVER_NT35510) ? 16 : 8,
        .lcd_param_bits    = (config->driver == BLUSYS_LCD_DRIVER_NT35510) ? 16 : 8,
        .spi_mode          = 0,
        .trans_queue_depth = 1,
    };

    esp_err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)host,
                                       &io_cfg, &lcd->io_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        vSemaphoreDelete(lcd->color_done_sem);
        lcd->color_done_sem = NULL;
        lcd->wait_color_trans_done = false;
        spi_bus_free(host);
        lcd->spi_bus_owned = false;
        return err;
    }

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = config->spi.rst_pin,
        .rgb_ele_order  = config->bgr_order ? LCD_RGB_ELEMENT_ORDER_BGR
                                            : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = config->bits_per_pixel,
    };

    if (config->driver == BLUSYS_LCD_DRIVER_ST7789) {
        esp_err = esp_lcd_new_panel_st7789(lcd->io_handle, &panel_cfg,
                                           &lcd->panel_handle);
    } else if (config->driver == BLUSYS_LCD_DRIVER_ST7735) {
        esp_err = blusys_lcd_new_panel_st7735(lcd->io_handle, &panel_cfg,
                                              &lcd->panel_handle);
    } else {
        esp_err = esp_lcd_new_panel_nt35510(lcd->io_handle, &panel_cfg,
                                            &lcd->panel_handle);
    }

    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        esp_lcd_panel_io_del(lcd->io_handle);
        lcd->io_handle = NULL;
        vSemaphoreDelete(lcd->color_done_sem);
        lcd->color_done_sem = NULL;
        lcd->wait_color_trans_done = false;
        spi_bus_free(host);
        lcd->spi_bus_owned = false;
        return err;
    }

    return BLUSYS_OK;
}

static blusys_err_t blusys_lcd_open_i2c(const blusys_lcd_config_t *config,
                                        blusys_lcd_t *lcd)
{
    esp_err_t esp_err;
    blusys_err_t err;

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = (i2c_port_num_t)config->i2c.port,
        .sda_io_num        = (gpio_num_t)config->i2c.sda_pin,
        .scl_io_num        = (gpio_num_t)config->i2c.scl_pin,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err = i2c_new_master_bus(&bus_cfg, &lcd->i2c_bus);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    esp_lcd_panel_io_i2c_config_t io_cfg = {
        .dev_addr            = config->i2c.dev_addr,
        .scl_speed_hz        = config->i2c.freq_hz,
        .control_phase_bytes = 1,
        .dc_bit_offset       = 6,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
    };

    esp_err = esp_lcd_new_panel_io_i2c(lcd->i2c_bus, &io_cfg, &lcd->io_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        i2c_del_master_bus(lcd->i2c_bus);
        lcd->i2c_bus = NULL;
        return err;
    }

    esp_lcd_panel_ssd1306_config_t ssd1306_cfg = {
        .height = (uint8_t)config->height,
    };
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = config->i2c.rst_pin,
        .bits_per_pixel = 1,
        .vendor_config  = &ssd1306_cfg,
    };

    esp_err = esp_lcd_new_panel_ssd1306(lcd->io_handle, &panel_cfg,
                                        &lcd->panel_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        esp_lcd_panel_io_del(lcd->io_handle);
        lcd->io_handle = NULL;
        i2c_del_master_bus(lcd->i2c_bus);
        lcd->i2c_bus = NULL;
        return err;
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_lcd_open(const blusys_lcd_config_t *config,
                             blusys_lcd_t **out_lcd)
{
    blusys_lcd_t *lcd;
    blusys_err_t err;
    esp_err_t esp_err;
    bool is_spi_panel;

    if ((config == NULL) || (out_lcd == NULL) ||
        (config->width == 0u) || (config->height == 0u) ||
        (config->bits_per_pixel == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    is_spi_panel = config->driver != BLUSYS_LCD_DRIVER_SSD1306;

    switch (config->driver) {
    case BLUSYS_LCD_DRIVER_ST7789:
    case BLUSYS_LCD_DRIVER_NT35510:
    case BLUSYS_LCD_DRIVER_ST7735:
        if (!blusys_lcd_is_valid_spi_bus(config->spi.bus) ||
            !GPIO_IS_VALID_OUTPUT_GPIO(config->spi.sclk_pin) ||
            !GPIO_IS_VALID_OUTPUT_GPIO(config->spi.mosi_pin) ||
            !GPIO_IS_VALID_OUTPUT_GPIO(config->spi.cs_pin) ||
            !GPIO_IS_VALID_OUTPUT_GPIO(config->spi.dc_pin) ||
            (config->spi.pclk_hz == 0u)) {
            return BLUSYS_ERR_INVALID_ARG;
        }
        break;
    case BLUSYS_LCD_DRIVER_SSD1306:
        if (!GPIO_IS_VALID_OUTPUT_GPIO(config->i2c.sda_pin) ||
            !GPIO_IS_VALID_OUTPUT_GPIO(config->i2c.scl_pin) ||
            (config->i2c.port < 0) || (config->i2c.port >= SOC_I2C_NUM) ||
            (config->i2c.dev_addr > 0x7Fu) ||
            (config->i2c.freq_hz == 0u)) {
            return BLUSYS_ERR_INVALID_ARG;
        }
        break;
    default:
        return BLUSYS_ERR_INVALID_ARG;
    }

    lcd = calloc(1, sizeof(*lcd));
    if (lcd == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    lcd->driver = config->driver;
    lcd->bl_pin = -1;
    lcd->width  = config->width;
    lcd->height = config->height;

    err = blusys_lock_init(&lcd->lock);
    if (err != BLUSYS_OK) {
        free(lcd);
        return err;
    }

    if ((config->driver == BLUSYS_LCD_DRIVER_ST7789) ||
        (config->driver == BLUSYS_LCD_DRIVER_NT35510) ||
        (config->driver == BLUSYS_LCD_DRIVER_ST7735)) {
        lcd->bl_pin = config->spi.bl_pin;
        err = blusys_lcd_open_spi(config, lcd);
    } else {
        err = blusys_lcd_open_i2c(config, lcd);
    }
    if (err != BLUSYS_OK) {
        blusys_lock_deinit(&lcd->lock);
        free(lcd);
        return err;
    }

    esp_err = esp_lcd_panel_reset(lcd->panel_handle);
    if (esp_err == ESP_OK) {
        esp_err = esp_lcd_panel_init(lcd->panel_handle);
    }
    if (esp_err == ESP_OK && is_spi_panel &&
        (config->spi.x_offset || config->spi.y_offset)) {
        esp_err = esp_lcd_panel_set_gap(lcd->panel_handle,
                                        config->spi.x_offset,
                                        config->spi.y_offset);
    }
    if (esp_err == ESP_OK) {
        esp_err = esp_lcd_panel_swap_xy(lcd->panel_handle, config->swap_xy);
    }
    if (esp_err == ESP_OK) {
        esp_err = esp_lcd_panel_mirror(lcd->panel_handle,
                                       config->mirror_x,
                                       config->mirror_y);
    }
    if (esp_err == ESP_OK) {
        esp_err = esp_lcd_panel_invert_color(lcd->panel_handle,
                                             config->invert_color);
    }
    if (esp_err == ESP_OK) {
        esp_err = esp_lcd_panel_disp_on_off(lcd->panel_handle, true);
    }
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lcd_teardown(lcd);
        return err;
    }

    if (lcd->bl_pin != -1) {
        gpio_config_t bl_cfg = {
            .pin_bit_mask = 1ULL << (uint32_t)lcd->bl_pin,
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        esp_err = gpio_config(&bl_cfg);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            blusys_lcd_teardown(lcd);
            return err;
        }
        gpio_set_level((gpio_num_t)lcd->bl_pin, 1);
    }

    *out_lcd = lcd;
    return BLUSYS_OK;
}

blusys_err_t blusys_lcd_close(blusys_lcd_t *lcd)
{
    blusys_err_t err;

    if (lcd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_lcd_panel_disp_on_off(lcd->panel_handle, false);
    blusys_lock_give(&lcd->lock);
    blusys_lcd_teardown(lcd);
    return BLUSYS_OK;
}

blusys_err_t blusys_lcd_draw_bitmap(blusys_lcd_t *lcd,
                                    int x_start, int y_start,
                                    int x_end, int y_end,
                                    const void *color_data)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if ((lcd == NULL) || (color_data == NULL) ||
        (x_start < 0) || (y_start < 0) ||
        (x_end <= x_start) || (y_end <= y_start)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = esp_lcd_panel_draw_bitmap(lcd->panel_handle,
                                        x_start, y_start,
                                        x_end, y_end,
                                        color_data);
    if ((esp_err == ESP_OK) &&
        lcd->wait_color_trans_done &&
        (lcd->color_done_sem != NULL)) {
        if (xSemaphoreTake(lcd->color_done_sem, portMAX_DELAY) != pdTRUE) {
            err = BLUSYS_ERR_TIMEOUT;
        } else {
            /* Drain callbacks from any remaining chunks in a multi-chunk transfer. */
            while (xSemaphoreTake(lcd->color_done_sem, 0) == pdTRUE) {}
            err = BLUSYS_OK;
        }
    } else {
        err = blusys_translate_esp_err(esp_err);
    }

    blusys_lock_give(&lcd->lock);
    return err;
}

blusys_err_t blusys_lcd_on(blusys_lcd_t *lcd)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (lcd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = esp_lcd_panel_disp_on_off(lcd->panel_handle, true);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&lcd->lock);
    return err;
}

blusys_err_t blusys_lcd_off(blusys_lcd_t *lcd)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (lcd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = esp_lcd_panel_disp_on_off(lcd->panel_handle, false);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&lcd->lock);
    return err;
}

blusys_err_t blusys_lcd_mirror(blusys_lcd_t *lcd, bool mirror_x, bool mirror_y)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (lcd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = esp_lcd_panel_mirror(lcd->panel_handle, mirror_x, mirror_y);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&lcd->lock);
    return err;
}

blusys_err_t blusys_lcd_swap_xy(blusys_lcd_t *lcd, bool swap)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (lcd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = esp_lcd_panel_swap_xy(lcd->panel_handle, swap);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&lcd->lock);
    return err;
}

blusys_err_t blusys_lcd_invert_colors(blusys_lcd_t *lcd, bool invert)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (lcd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = esp_lcd_panel_invert_color(lcd->panel_handle, invert);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&lcd->lock);
    return err;
}

blusys_err_t blusys_lcd_set_brightness(blusys_lcd_t *lcd, int percent)
{
    blusys_err_t err;

    if ((lcd == NULL) || (percent < 0) || (percent > 100)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&lcd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (lcd->bl_pin != -1) {
        gpio_set_level((gpio_num_t)lcd->bl_pin, percent > 0 ? 1 : 0);
    }

    blusys_lock_give(&lcd->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_lcd_get_dimensions(blusys_lcd_t *lcd,
                                       uint32_t *width, uint32_t *height)
{
    if (lcd == NULL || (width == NULL && height == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (width != NULL) {
        *width = lcd->width;
    }
    if (height != NULL) {
        *height = lcd->height;
    }
    return BLUSYS_OK;
}
