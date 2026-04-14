#include "blusys/hal/spi_slave.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/spi_slave.h"

struct blusys_spi_slave {
    spi_host_device_t host;
    blusys_lock_t     lock;
};

static bool blusys_spi_slave_is_valid_mode(uint8_t mode)
{
    return mode <= 3u;
}

blusys_err_t blusys_spi_slave_open(int mosi_pin,
                                    int miso_pin,
                                    int clk_pin,
                                    int cs_pin,
                                    uint8_t mode,
                                    blusys_spi_slave_t **out_slave)
{
    blusys_spi_slave_t *slave;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!GPIO_IS_VALID_GPIO(mosi_pin) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(miso_pin) ||
        !GPIO_IS_VALID_GPIO(clk_pin) ||
        !GPIO_IS_VALID_GPIO(cs_pin) ||
        !blusys_spi_slave_is_valid_mode(mode) ||
        (out_slave == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    slave = calloc(1, sizeof(*slave));
    if (slave == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&slave->lock);
    if (err != BLUSYS_OK) {
        free(slave);
        return err;
    }

    slave->host = SPI2_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = mosi_pin,
        .miso_io_num     = miso_pin,
        .sclk_io_num     = clk_pin,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 0,
    };
    spi_slave_interface_config_t slave_cfg = {
        .spics_io_num  = cs_pin,
        .flags         = 0u,
        .queue_size    = 1,
        .mode          = mode,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL,
    };

    esp_err = spi_slave_initialize(slave->host, &bus_cfg, &slave_cfg, SPI_DMA_CH_AUTO);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&slave->lock);
        free(slave);
        return err;
    }

    *out_slave = slave;
    return BLUSYS_OK;
}

blusys_err_t blusys_spi_slave_close(blusys_spi_slave_t *slave)
{
    blusys_err_t err;

    if (slave == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&slave->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(spi_slave_free(slave->host));

    blusys_lock_give(&slave->lock);

    if (err == BLUSYS_OK) {
        blusys_lock_deinit(&slave->lock);
        free(slave);
    }

    return err;
}

blusys_err_t blusys_spi_slave_transfer(blusys_spi_slave_t *slave,
                                        const void *tx_buf,
                                        void *rx_buf,
                                        size_t size,
                                        int timeout_ms)
{
    blusys_err_t err;
    spi_slave_transaction_t t = {
        .length    = size * 8u,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };
    TickType_t ticks;

    if ((slave == NULL) || (size == 0u) ||
        ((tx_buf == NULL) && (rx_buf == NULL)) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    ticks = (timeout_ms < 0) ? portMAX_DELAY : (TickType_t) pdMS_TO_TICKS(timeout_ms);

    err = blusys_lock_take(&slave->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(spi_slave_transmit(slave->host, &t, ticks));

    blusys_lock_give(&slave->lock);
    return err;
}
