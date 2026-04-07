#include "blusys/lcd.h"

#include <stdbool.h>
#include <stdlib.h>

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_nt35510.h"

struct blusys_lcd {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t    panel_handle;
    blusys_lock_t             lock;
    int                       bl_pin;
    blusys_lcd_driver_t       driver;
    spi_host_device_t         spi_host;
    i2c_master_bus_handle_t   i2c_bus;
    bool                      spi_bus_owned;
};

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

    spi_bus_config_t bus_cfg = {
        .mosi_io_num   = config->spi.mosi_pin,
        .miso_io_num   = -1,
        .sclk_io_num   = config->spi.sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)((config->width * 80u * config->bits_per_pixel + 7u) / 8u),
    };

    esp_err = spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    lcd->spi_host = host;
    lcd->spi_bus_owned = true;

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = config->spi.dc_pin,
        .cs_gpio_num       = config->spi.cs_pin,
        .pclk_hz           = config->spi.pclk_hz,
        .lcd_cmd_bits      = (config->driver == BLUSYS_LCD_DRIVER_NT35510) ? 16 : 8,
        .lcd_param_bits    = (config->driver == BLUSYS_LCD_DRIVER_NT35510) ? 16 : 8,
        .spi_mode          = 0,
        .trans_queue_depth = 1,
    };

    esp_err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)host,
                                       &io_cfg, &lcd->io_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
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
    } else {
        esp_err = esp_lcd_new_panel_nt35510(lcd->io_handle, &panel_cfg,
                                            &lcd->panel_handle);
    }

    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        esp_lcd_panel_io_del(lcd->io_handle);
        lcd->io_handle = NULL;
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

    if ((config == NULL) || (out_lcd == NULL) ||
        (config->width == 0u) || (config->height == 0u) ||
        (config->bits_per_pixel == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    switch (config->driver) {
    case BLUSYS_LCD_DRIVER_ST7789:
    case BLUSYS_LCD_DRIVER_NT35510:
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

    err = blusys_lock_init(&lcd->lock);
    if (err != BLUSYS_OK) {
        free(lcd);
        return err;
    }

    if ((config->driver == BLUSYS_LCD_DRIVER_ST7789) ||
        (config->driver == BLUSYS_LCD_DRIVER_NT35510)) {
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
    err = blusys_translate_esp_err(esp_err);

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
