#include "blusys/i2c.h"

#include <stdbool.h>
#include <stdlib.h>

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "soc/soc_caps.h"

struct blusys_i2c_master {
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t device_handle;
    uint16_t device_address;
    uint32_t freq_hz;
    blusys_lock_t lock;
};

static bool blusys_i2c_is_valid_port(int port)
{
    return (port >= 0) && ((unsigned int) port < SOC_I2C_NUM);
}

static bool blusys_i2c_is_valid_signal_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_i2c_is_valid_address(uint16_t device_address)
{
    return device_address <= 0x7fu;
}

static bool blusys_i2c_is_valid_scan_address(uint16_t device_address)
{
    return (device_address >= 0x08u) && (device_address <= 0x77u);
}

static blusys_err_t blusys_i2c_ensure_device(blusys_i2c_master_t *i2c, uint16_t device_address, int timeout_ms)
{
    esp_err_t esp_err;

    if (i2c->device_handle == NULL) {
        i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = device_address,
            .scl_speed_hz = i2c->freq_hz,
        };

        esp_err = i2c_master_bus_add_device(i2c->bus_handle, &device_config, &i2c->device_handle);
        if (esp_err != ESP_OK) {
            return blusys_translate_esp_err(esp_err);
        }

        i2c->device_address = device_address;
        return BLUSYS_OK;
    }

    if (i2c->device_address == device_address) {
        return BLUSYS_OK;
    }

    esp_err = i2c_master_device_change_address(i2c->device_handle, device_address, timeout_ms);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    i2c->device_address = device_address;
    return BLUSYS_OK;
}

blusys_err_t blusys_i2c_master_open(int port,
                                    int sda_pin,
                                    int scl_pin,
                                    uint32_t freq_hz,
                                    blusys_i2c_master_t **out_i2c)
{
    i2c_master_bus_config_t config = {
        .i2c_port = (i2c_port_num_t) port,
        .sda_io_num = (gpio_num_t) sda_pin,
        .scl_io_num = (gpio_num_t) scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .trans_queue_depth = 1,
        .flags.enable_internal_pullup = true,
    };
    blusys_i2c_master_t *i2c;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((!blusys_i2c_is_valid_port(port)) ||
        (!blusys_i2c_is_valid_signal_pin(sda_pin)) ||
        (!blusys_i2c_is_valid_signal_pin(scl_pin)) ||
        (freq_hz == 0u) ||
        (out_i2c == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    i2c = calloc(1, sizeof(*i2c));
    if (i2c == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&i2c->lock);
    if (err != BLUSYS_OK) {
        free(i2c);
        return err;
    }

    i2c->freq_hz = freq_hz;

    esp_err = i2c_new_master_bus(&config, &i2c->bus_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&i2c->lock);
        free(i2c);
        return err;
    }

    *out_i2c = i2c;
    return BLUSYS_OK;
}

blusys_err_t blusys_i2c_master_close(blusys_i2c_master_t *i2c)
{
    blusys_err_t err;
    esp_err_t esp_err = ESP_OK;

    if (i2c == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2c->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (i2c->device_handle != NULL) {
        esp_err = i2c_master_bus_rm_device(i2c->device_handle);
        if (esp_err == ESP_OK) {
            i2c->device_handle = NULL;
        }
    }

    if (esp_err == ESP_OK) {
        esp_err = i2c_del_master_bus(i2c->bus_handle);
    }

    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&i2c->lock);

    if (err == BLUSYS_OK) {
        blusys_lock_deinit(&i2c->lock);
        free(i2c);
    }

    return err;
}

blusys_err_t blusys_i2c_master_probe(blusys_i2c_master_t *i2c, uint16_t device_address, int timeout_ms)
{
    blusys_err_t err;

    if ((i2c == NULL) || !blusys_i2c_is_valid_address(device_address) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2c->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(i2c_master_probe(i2c->bus_handle, device_address, timeout_ms));

    blusys_lock_give(&i2c->lock);

    return err;
}

blusys_err_t blusys_i2c_master_scan(blusys_i2c_master_t *i2c,
                                    uint16_t start_address,
                                    uint16_t end_address,
                                    int timeout_ms,
                                    blusys_i2c_scan_callback_t callback,
                                    void *user_ctx,
                                    size_t *out_count)
{
    blusys_err_t err;
    esp_err_t esp_err;
    size_t count = 0u;
    uint16_t device_address;

    if ((i2c == NULL) ||
        !blusys_i2c_is_valid_scan_address(start_address) ||
        !blusys_i2c_is_valid_scan_address(end_address) ||
        (start_address > end_address) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2c->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    for (device_address = start_address; device_address <= end_address; ++device_address) {
        esp_err = i2c_master_probe(i2c->bus_handle, device_address, timeout_ms);
        if (esp_err == ESP_OK) {
            count += 1u;

            if ((callback != NULL) && !callback(device_address, user_ctx)) {
                break;
            }

            continue;
        }

        if (esp_err != ESP_ERR_NOT_FOUND) {
            blusys_lock_give(&i2c->lock);

            if (out_count != NULL) {
                *out_count = count;
            }

            return blusys_translate_esp_err(esp_err);
        }
    }

    blusys_lock_give(&i2c->lock);

    if (out_count != NULL) {
        *out_count = count;
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_i2c_master_write(blusys_i2c_master_t *i2c,
                                     uint16_t device_address,
                                     const void *data,
                                     size_t size,
                                     int timeout_ms)
{
    blusys_err_t err;

    if ((i2c == NULL) ||
        !blusys_i2c_is_valid_address(device_address) ||
        ((data == NULL) && (size != 0u)) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&i2c->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_i2c_ensure_device(i2c, device_address, timeout_ms);
    if (err == BLUSYS_OK) {
        err = blusys_translate_esp_err(i2c_master_transmit(i2c->device_handle,
                                                           (const uint8_t *) data,
                                                           size,
                                                           timeout_ms));
    }

    blusys_lock_give(&i2c->lock);

    return err;
}

blusys_err_t blusys_i2c_master_read(blusys_i2c_master_t *i2c,
                                    uint16_t device_address,
                                    void *data,
                                    size_t size,
                                    int timeout_ms)
{
    blusys_err_t err;

    if ((i2c == NULL) ||
        !blusys_i2c_is_valid_address(device_address) ||
        ((data == NULL) && (size != 0u)) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&i2c->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_i2c_ensure_device(i2c, device_address, timeout_ms);
    if (err == BLUSYS_OK) {
        err = blusys_translate_esp_err(i2c_master_receive(i2c->device_handle,
                                                          (uint8_t *) data,
                                                          size,
                                                          timeout_ms));
    }

    blusys_lock_give(&i2c->lock);

    return err;
}

blusys_err_t blusys_i2c_master_write_read(blusys_i2c_master_t *i2c,
                                          uint16_t device_address,
                                          const void *tx_data,
                                          size_t tx_size,
                                          void *rx_data,
                                          size_t rx_size,
                                          int timeout_ms)
{
    blusys_err_t err;

    if ((tx_size == 0u) && (rx_size == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (tx_size == 0u) {
        return blusys_i2c_master_read(i2c, device_address, rx_data, rx_size, timeout_ms);
    }

    if (rx_size == 0u) {
        return blusys_i2c_master_write(i2c, device_address, tx_data, tx_size, timeout_ms);
    }

    if ((i2c == NULL) ||
        !blusys_i2c_is_valid_address(device_address) ||
        (tx_data == NULL) ||
        (rx_data == NULL) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2c->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_i2c_ensure_device(i2c, device_address, timeout_ms);
    if (err == BLUSYS_OK) {
        err = blusys_translate_esp_err(i2c_master_transmit_receive(i2c->device_handle,
                                                                   (const uint8_t *) tx_data,
                                                                   tx_size,
                                                                   (uint8_t *) rx_data,
                                                                   rx_size,
                                                                   timeout_ms));
    }

    blusys_lock_give(&i2c->lock);

    return err;
}
