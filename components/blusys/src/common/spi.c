#include "blusys/spi.h"

#include <stdbool.h>
#include <stdlib.h>

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

struct blusys_spi {
    spi_host_device_t host;
    spi_device_handle_t device;
    blusys_lock_t lock;
};

static bool blusys_spi_is_valid_bus(int bus)
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

static spi_host_device_t blusys_spi_bus_to_host(int bus)
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

static bool blusys_spi_is_valid_output_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_spi_is_valid_input_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

blusys_err_t blusys_spi_open(int bus,
                             int sclk_pin,
                             int mosi_pin,
                             int miso_pin,
                             int cs_pin,
                             uint32_t freq_hz,
                             blusys_spi_t **out_spi)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MISO,
    };
    spi_device_interface_config_t device_config = {
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = (int) freq_hz,
        .spics_io_num = cs_pin,
        .queue_size = 1,
    };
    blusys_spi_t *spi;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!blusys_spi_is_valid_bus(bus) ||
        !blusys_spi_is_valid_output_pin(sclk_pin) ||
        !blusys_spi_is_valid_output_pin(mosi_pin) ||
        !blusys_spi_is_valid_input_pin(miso_pin) ||
        !blusys_spi_is_valid_output_pin(cs_pin) ||
        (freq_hz == 0u) ||
        (out_spi == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    spi = calloc(1, sizeof(*spi));
    if (spi == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&spi->lock);
    if (err != BLUSYS_OK) {
        free(spi);
        return err;
    }

    spi->host = blusys_spi_bus_to_host(bus);

    esp_err = spi_bus_initialize(spi->host, &bus_config, SPI_DMA_CH_AUTO);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&spi->lock);
        free(spi);
        return err;
    }

    esp_err = spi_bus_add_device(spi->host, &device_config, &spi->device);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        spi_bus_free(spi->host);
        blusys_lock_deinit(&spi->lock);
        free(spi);
        return err;
    }

    *out_spi = spi;
    return BLUSYS_OK;
}

blusys_err_t blusys_spi_close(blusys_spi_t *spi)
{
    blusys_err_t err;
    esp_err_t esp_err = ESP_OK;

    if (spi == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&spi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (spi->device != NULL) {
        esp_err = spi_bus_remove_device(spi->device);
        if (esp_err == ESP_OK) {
            spi->device = NULL;
        }
    }

    if (esp_err == ESP_OK) {
        esp_err = spi_bus_free(spi->host);
    }

    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&spi->lock);

    if (err == BLUSYS_OK) {
        blusys_lock_deinit(&spi->lock);
        free(spi);
    }

    return err;
}

blusys_err_t blusys_spi_transfer(blusys_spi_t *spi,
                                 const void *tx_data,
                                 void *rx_data,
                                 size_t size)
{
    spi_transaction_t transaction = {
        .length = size * 8u,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    blusys_err_t err;
    esp_err_t esp_err;

    if ((spi == NULL) || ((tx_data == NULL) && (rx_data == NULL) && (size != 0u))) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&spi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = spi_device_transmit(spi->device, &transaction);

    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&spi->lock);

    return err;
}
