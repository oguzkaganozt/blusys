/**
 * @file spi_slave.h
 * @brief Synchronous SPI slave: full-duplex exchange with a master, blocking with timeout.
 *
 * Only SPI2_HOST is used; a second simultaneous slave is not supported. See
 * docs/hal/spi.md (Slave Mode section) for modes and caveats.
 */

#ifndef BLUSYS_SPI_SLAVE_H
#define BLUSYS_SPI_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for the SPI slave peripheral. */
typedef struct blusys_spi_slave blusys_spi_slave_t;

/**
 * @brief Initialize the SPI slave (DMA enabled automatically).
 *
 * `mode` selects the SPI clock mode and must match the master:
 * | `mode` | CPOL | CPHA | Clock idle / sample edge |
 * |--------|------|------|--------------------------|
 * | 0      | 0    | 0    | idle low / rising        |
 * | 1      | 0    | 1    | idle low / falling       |
 * | 2      | 1    | 0    | idle high / falling      |
 * | 3      | 1    | 1    | idle high / rising       |
 *
 * @param mosi_pin   GPIO for MOSI.
 * @param miso_pin   GPIO for MISO.
 * @param clk_pin    GPIO for clock.
 * @param cs_pin     GPIO for chip select.
 * @param mode       SPI clock mode (0–3); must match the master.
 * @param out_slave  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins or unsupported mode.
 */
blusys_err_t blusys_spi_slave_open(int mosi_pin,
                                    int miso_pin,
                                    int clk_pin,
                                    int cs_pin,
                                    uint8_t mode,
                                    blusys_spi_slave_t **out_slave);

/**
 * @brief Deinitialize the SPI slave and free the handle.
 * @param slave Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p slave is NULL.
 */
blusys_err_t blusys_spi_slave_close(blusys_spi_slave_t *slave);

/**
 * @brief Block until the master initiates a transfer and exchange @p size bytes.
 *
 * For half-duplex operation, one of @p tx_buf or @p rx_buf may be NULL, but not both.
 *
 * @param slave       Handle.
 * @param tx_buf      Data to send to master; may be NULL for receive-only.
 * @param rx_buf      Buffer for data from master; may be NULL for send-only.
 * @param size        Number of bytes to exchange.
 * @param timeout_ms  Milliseconds to wait for master to initiate; use
 *                    `BLUSYS_TIMEOUT_FOREVER` to block indefinitely.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if master doesn't initiate in time.
 */
blusys_err_t blusys_spi_slave_transfer(blusys_spi_slave_t *slave,
                                        const void *tx_buf,
                                        void *rx_buf,
                                        size_t size,
                                        int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
