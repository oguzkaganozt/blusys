#ifndef BLUSYS_SPI_H
#define BLUSYS_SPI_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_spi blusys_spi_t;

blusys_err_t blusys_spi_open(int bus,
                             int sclk_pin,
                             int mosi_pin,
                             int miso_pin,
                             int cs_pin,
                             uint32_t freq_hz,
                             blusys_spi_t **out_spi);
blusys_err_t blusys_spi_close(blusys_spi_t *spi);
blusys_err_t blusys_spi_transfer(blusys_spi_t *spi,
                                 const void *tx_data,
                                 void *rx_data,
                                 size_t size);

#ifdef __cplusplus
}
#endif

#endif
