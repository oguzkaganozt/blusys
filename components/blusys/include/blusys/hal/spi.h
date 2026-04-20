/**
 * @file spi.h
 * @brief Blocking full-duplex SPI master for single-device transfers. SPI mode 0.
 *
 * Each handle owns one bus and one attached device. See docs/hal/spi.md.
 */

#ifndef BLUSYS_SPI_H
#define BLUSYS_SPI_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle owning one SPI bus + one chip-select device. */
typedef struct blusys_spi blusys_spi_t;

/**
 * @brief Initialize an SPI bus and register one device on it. SPI mode 0 is used.
 *
 * @param bus        SPI bus index (target-dependent; typically 0 or 1).
 * @param sclk_pin   GPIO for clock.
 * @param mosi_pin   GPIO for MOSI.
 * @param miso_pin   GPIO for MISO; pass `-1` if not used.
 * @param cs_pin     GPIO for chip select.
 * @param freq_hz    Bus clock frequency.
 * @param out_spi    Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid arguments,
 *         `BLUSYS_ERR_INVALID_STATE` if the bus is already in use.
 */
blusys_err_t blusys_spi_open(int bus,
                             int sclk_pin,
                             int mosi_pin,
                             int miso_pin,
                             int cs_pin,
                             uint32_t freq_hz,
                             blusys_spi_t **out_spi);

/**
 * @brief Remove the attached device, free the bus, release the handle.
 * @param spi Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p spi is NULL.
 */
blusys_err_t blusys_spi_close(blusys_spi_t *spi);

/**
 * @brief Perform a full-duplex transfer.
 *
 * @p tx_data and @p rx_data may point to the same buffer for in-place exchange.
 * Pass NULL for @p rx_data if received data is not needed.
 *
 * @param spi      Handle.
 * @param tx_data  Data to send; must not be NULL.
 * @param rx_data  Buffer for received data; may be NULL.
 * @param size     Number of bytes to transfer.
 * @return `BLUSYS_OK`, translated ESP-IDF error on transfer failure.
 */
blusys_err_t blusys_spi_transfer(blusys_spi_t *spi,
                                 const void *tx_data,
                                 void *rx_data,
                                 size_t size);

#ifdef __cplusplus
}
#endif

#endif
