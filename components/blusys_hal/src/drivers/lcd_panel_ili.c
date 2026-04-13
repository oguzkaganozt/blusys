/*
 * ILI9341 / ILI9488 SPI panel factories for Blusys HAL.
 *
 * ESP-IDF 5.5.x does not ship esp_lcd_panel_ili9341 / ili9488 in esp_lcd; these
 * implementations follow the same contract as esp_lcd_panel_st7789 (CASET/RASET/RAMWR).
 */

#include "blusys/internal/lcd_panel_ili.h"

#include <stddef.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"

typedef enum {
    BLUSYS_ILI_KIND_9341 = 0,
    BLUSYS_ILI_KIND_9488 = 1,
} blusys_ili_kind_t;

typedef struct {
    esp_lcd_panel_t           base;
    esp_lcd_panel_io_handle_t io;
    int                       reset_gpio_num;
    bool                      reset_level;
    int                       x_gap;
    int                       y_gap;
    uint8_t                   fb_bits_per_pixel;
    uint8_t                   madctl_val;
    uint8_t                   colmod_val;
    blusys_ili_kind_t         kind;
} blusys_ili_panel_t;

static const char *TAG = "blusys.lcd.ili";

static blusys_ili_panel_t *ili_from_panel(esp_lcd_panel_t *panel)
{
    return (blusys_ili_panel_t *)((char *)panel - offsetof(blusys_ili_panel_t, base));
}

static esp_err_t panel_ili_del(esp_lcd_panel_t *panel);
static esp_err_t panel_ili_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_ili_init(esp_lcd_panel_t *panel);
static esp_err_t panel_ili_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start,
                                       int x_end, int y_end, const void *color_data);
static esp_err_t panel_ili_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_ili_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_ili_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_ili_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_ili_set_brightness(esp_lcd_panel_t *panel, int brightness);
static esp_err_t panel_ili_disp_on_off(esp_lcd_panel_t *panel, bool on_off);
static esp_err_t panel_ili_sleep(esp_lcd_panel_t *panel, bool sleep);

static esp_err_t ili9341_chip_init(blusys_ili_panel_t *m)
{
    esp_lcd_panel_io_handle_t io = m->io;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xEF, (uint8_t[]){0x03, 0x80, 0x02}, 3), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xCF, (uint8_t[]){0x00, 0xC1, 0x30}, 3), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xED, (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE8, (uint8_t[]){0x85, 0x00, 0x78}, 3), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5), TAG,
                        "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xF7, (uint8_t[]){0x20}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xEA, (uint8_t[]){0x00, 0x00}, 2), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC0, (uint8_t[]){0x23}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC1, (uint8_t[]){0x10}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC5, (uint8_t[]){0x3E, 0x28}, 2), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC7, (uint8_t[]){0x86}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &m->madctl_val, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x33, (uint8_t[]){0x00}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, &m->colmod_val, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB1, (uint8_t[]){0x00, 0x18}, 2), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB6, (uint8_t[]){0x08, 0x82, 0x27}, 3), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xF2, (uint8_t[]){0x00}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x26, (uint8_t[]){0x01}, 1), TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE0,
                                                  (uint8_t[]){0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                                                              0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00},
                                                  15),
                        TAG, "ili9341");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE1,
                                                  (uint8_t[]){0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                                                              0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F},
                                                  15),
                        TAG, "ili9341");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0), TAG, "ili9341");
    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_DISPON, NULL, 0), TAG, "ili9341");
    return ESP_OK;
}

static esp_err_t ili9488_chip_init(blusys_ili_panel_t *m)
{
    esp_lcd_panel_io_handle_t io = m->io;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE0,
                                                  (uint8_t[]){0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78,
                                                              0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F},
                                                  15),
                        TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE1,
                                                  (uint8_t[]){0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45,
                                                              0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F},
                                                  15),
                        TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC0, (uint8_t[]){0x17, 0x15}, 2), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC1, (uint8_t[]){0x41}, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC5, (uint8_t[]){0x00, 0x12, 0x80}, 3), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &m->madctl_val, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, &m->colmod_val, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB0, (uint8_t[]){0x00}, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB1, (uint8_t[]){0xA0}, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB4, (uint8_t[]){0x02}, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB6, (uint8_t[]){0x02, 0x02, 0x3B}, 3), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB7, (uint8_t[]){0xC6}, 1), TAG, "ili9488");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xF7, (uint8_t[]){0xA9, 0x51, 0x2C, 0x82}, 4), TAG,
                        "ili9488");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0), TAG, "ili9488");
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_DISPON, NULL, 0), TAG, "ili9488");
    vTaskDelay(pdMS_TO_TICKS(25));
    return ESP_OK;
}

static esp_err_t panel_ili_del(esp_lcd_panel_t *panel)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);

    if (m->reset_gpio_num >= 0) {
        gpio_reset_pin((gpio_num_t)m->reset_gpio_num);
    }
    free(m);
    return ESP_OK;
}

static esp_err_t panel_ili_reset(esp_lcd_panel_t *panel)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);
    esp_lcd_panel_io_handle_t io = m->io;

    if (m->reset_gpio_num >= 0) {
        gpio_set_level((gpio_num_t)m->reset_gpio_num, m->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)m->reset_gpio_num, !m->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), TAG, "reset");
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return ESP_OK;
}

static esp_err_t panel_ili_init(esp_lcd_panel_t *panel)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);

    if (m->kind == BLUSYS_ILI_KIND_9341) {
        return ili9341_chip_init(m);
    }
    return ili9488_chip_init(m);
}

static esp_err_t panel_ili_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start,
                                       int x_end, int y_end, const void *color_data)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);
    esp_lcd_panel_io_handle_t io = m->io;

    x_start += m->x_gap;
    x_end += m->x_gap;
    y_start += m->y_gap;
    y_end += m->y_gap;

    const size_t row_bytes = (size_t)(x_end - x_start) * (size_t)m->fb_bits_per_pixel / 8u;
    const uint8_t *src = color_data;

    /* One display row per RAMWR, matching the ST7735 path. Multi-row RASET
     * windows plus a long pixel burst have been observed to produce diagonal
     * shear on ESP32-C3 SPI; single-row transfers keep each DMA payload
     * small and re-assert COLMOD between rows to prevent drift. */
    for (int y = y_start; y < y_end; ++y) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, &m->colmod_val, 1),
                            TAG, "colmod");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]){
                                                          (x_start >> 8) & 0xFF,
                                                          x_start & 0xFF,
                                                          ((x_end - 1) >> 8) & 0xFF,
                                                          (x_end - 1) & 0xFF,
                                                      }, 4),
                            TAG, "caset");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]){
                                                          (y >> 8) & 0xFF,
                                                          y & 0xFF,
                                                          (y >> 8) & 0xFF,
                                                          y & 0xFF,
                                                      }, 4),
                            TAG, "raset");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, src, row_bytes),
                            TAG, "ramwr");
        src += row_bytes;
    }

    /* Small sync param tx flushes any async color tx before the source buffer
     * (e.g. LVGL flush scratch) can be reused by the caller. */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, &m->colmod_val, 1),
                        TAG, "sync");
    return ESP_OK;
}

static esp_err_t panel_ili_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);
    int command = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;
    return esp_lcd_panel_io_tx_param(m->io, command, NULL, 0);
}

static esp_err_t panel_ili_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);

    if (mirror_x) {
        m->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        m->madctl_val &= (uint8_t)~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        m->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        m->madctl_val &= (uint8_t)~LCD_CMD_MY_BIT;
    }
    return esp_lcd_panel_io_tx_param(m->io, LCD_CMD_MADCTL, &m->madctl_val, 1);
}

static esp_err_t panel_ili_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);

    if (swap_axes) {
        m->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        m->madctl_val &= (uint8_t)~LCD_CMD_MV_BIT;
    }
    return esp_lcd_panel_io_tx_param(m->io, LCD_CMD_MADCTL, &m->madctl_val, 1);
}

static esp_err_t panel_ili_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);
    m->x_gap = x_gap;
    m->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_ili_set_brightness(esp_lcd_panel_t *panel, int brightness)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);

    ESP_RETURN_ON_FALSE(brightness >= 0 && brightness <= 0xFF, ESP_ERR_INVALID_ARG, TAG, "brightness");
    uint8_t v = (uint8_t)brightness;
    return esp_lcd_panel_io_tx_param(m->io, LCD_CMD_WRDISBV, &v, 1);
}

static esp_err_t panel_ili_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);
    int command = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;
    return esp_lcd_panel_io_tx_param(m->io, command, NULL, 0);
}

static esp_err_t panel_ili_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    blusys_ili_panel_t *m = ili_from_panel(panel);
    int command = sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(m->io, command, NULL, 0), TAG, "sleep");
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

static esp_err_t ili_panel_alloc(blusys_ili_kind_t kind, const esp_lcd_panel_io_handle_t io,
                                 const esp_lcd_panel_dev_config_t *panel_dev_config,
                                 esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "arg");

    blusys_ili_panel_t *m = calloc(1, sizeof(blusys_ili_panel_t));
    ESP_RETURN_ON_FALSE(m, ESP_ERR_NO_MEM, TAG, "mem");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << (unsigned)panel_dev_config->reset_gpio_num,
        };
        if (gpio_config(&io_conf) != ESP_OK) {
            free(m);
            return ESP_FAIL;
        }
    }

    switch (panel_dev_config->rgb_ele_order) {
    case LCD_RGB_ELEMENT_ORDER_RGB:
        m->madctl_val = 0;
        break;
    case LCD_RGB_ELEMENT_ORDER_BGR:
        m->madctl_val = LCD_CMD_BGR_BIT;
        break;
    default:
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin((gpio_num_t)panel_dev_config->reset_gpio_num);
        }
        free(m);
        return ESP_ERR_NOT_SUPPORTED;
    }

    switch (panel_dev_config->bits_per_pixel) {
    case 16:
        m->colmod_val = 0x55;
        m->fb_bits_per_pixel = 16;
        break;
    case 18:
        m->colmod_val = 0x66;
        m->fb_bits_per_pixel = 24;
        break;
    default:
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin((gpio_num_t)panel_dev_config->reset_gpio_num);
        }
        free(m);
        return ESP_ERR_NOT_SUPPORTED;
    }

    m->kind             = kind;
    m->io               = io;
    m->reset_gpio_num   = panel_dev_config->reset_gpio_num;
    m->reset_level      = panel_dev_config->flags.reset_active_high;
    m->base.del         = panel_ili_del;
    m->base.reset       = panel_ili_reset;
    m->base.init        = panel_ili_init;
    m->base.draw_bitmap = panel_ili_draw_bitmap;
    m->base.invert_color = panel_ili_invert_color;
    m->base.mirror      = panel_ili_mirror;
    m->base.swap_xy     = panel_ili_swap_xy;
    m->base.set_gap     = panel_ili_set_gap;
    m->base.set_brightness = panel_ili_set_brightness;
    m->base.disp_on_off = panel_ili_disp_on_off;
    m->base.disp_sleep  = panel_ili_sleep;
    *ret_panel          = &m->base;
    return ESP_OK;
}

esp_err_t blusys_lcd_new_panel_ili9341(const esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_panel_dev_config_t *panel_dev_config,
                                       esp_lcd_panel_handle_t *ret_panel)
{
    return ili_panel_alloc(BLUSYS_ILI_KIND_9341, io, panel_dev_config, ret_panel);
}

esp_err_t blusys_lcd_new_panel_ili9488(const esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_panel_dev_config_t *panel_dev_config,
                                       esp_lcd_panel_handle_t *ret_panel)
{
    return ili_panel_alloc(BLUSYS_ILI_KIND_9488, io, panel_dev_config, ret_panel);
}
