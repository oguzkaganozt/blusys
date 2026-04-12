#ifndef BLUSYS_SPI_SLAVE_H
#define BLUSYS_SPI_SLAVE_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_spi_slave blusys_spi_slave_t;

blusys_err_t blusys_spi_slave_open(int mosi_pin,
                                    int miso_pin,
                                    int clk_pin,
                                    int cs_pin,
                                    uint8_t mode,
                                    blusys_spi_slave_t **out_slave);
blusys_err_t blusys_spi_slave_close(blusys_spi_slave_t *slave);
blusys_err_t blusys_spi_slave_transfer(blusys_spi_slave_t *slave,
                                        const void *tx_buf,
                                        void *rx_buf,
                                        size_t size,
                                        int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
