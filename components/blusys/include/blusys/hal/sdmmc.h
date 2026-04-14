#ifndef BLUSYS_SDMMC_H
#define BLUSYS_SDMMC_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_sdmmc blusys_sdmmc_t;

blusys_err_t blusys_sdmmc_open(int slot,
                               int clk_pin,
                               int cmd_pin,
                               int d0_pin,
                               int d1_pin,
                               int d2_pin,
                               int d3_pin,
                               int bus_width,
                               uint32_t freq_hz,
                               blusys_sdmmc_t **out_sdmmc);
blusys_err_t blusys_sdmmc_close(blusys_sdmmc_t *sdmmc);
blusys_err_t blusys_sdmmc_read_blocks(blusys_sdmmc_t *sdmmc,
                                      uint32_t start_block,
                                      void *buffer,
                                      uint32_t num_blocks,
                                      int timeout_ms);
blusys_err_t blusys_sdmmc_write_blocks(blusys_sdmmc_t *sdmmc,
                                       uint32_t start_block,
                                       const void *buffer,
                                       uint32_t num_blocks,
                                       int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
