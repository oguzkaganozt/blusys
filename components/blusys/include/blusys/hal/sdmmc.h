/**
 * @file sdmmc.h
 * @brief Raw block-level SDMMC driver (1-bit or 4-bit bus).
 *
 * Exposes `read_blocks` / `write_blocks` without any filesystem layer. Use
 * ::blusys_sd_spi for a FAT-backed VFS mount over SPI instead. See
 * docs/hal/sdmmc.md.
 */

#ifndef BLUSYS_SDMMC_H
#define BLUSYS_SDMMC_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque SDMMC peripheral handle. */
typedef struct blusys_sdmmc blusys_sdmmc_t;

/**
 * @brief Initialise the SDMMC peripheral and probe the card.
 *
 * @param slot       Host slot index (0 or 1, target-specific).
 * @param clk_pin    CLK GPIO pin.
 * @param cmd_pin    CMD GPIO pin.
 * @param d0_pin     DATA0 GPIO pin.
 * @param d1_pin     DATA1 GPIO pin (-1 to omit for 1-bit mode).
 * @param d2_pin     DATA2 GPIO pin (-1 to omit for 1-bit mode).
 * @param d3_pin     DATA3 GPIO pin (-1 to omit for 1-bit mode).
 * @param bus_width  1 or 4.
 * @param freq_hz    Bus clock in Hz; 0 selects a safe default.
 * @param out_sdmmc  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins or widths,
 *         `BLUSYS_ERR_NO_MEM`, `BLUSYS_ERR_IO` on probe failure.
 */
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

/**
 * @brief Release the SDMMC peripheral and free the handle.
 * @param sdmmc  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p sdmmc is NULL.
 */
blusys_err_t blusys_sdmmc_close(blusys_sdmmc_t *sdmmc);

/**
 * @brief Read @p num_blocks 512-byte blocks into @p buffer.
 *
 * @param sdmmc        Handle.
 * @param start_block  First block index.
 * @param buffer       Destination buffer (must hold `num_blocks * 512` bytes).
 * @param num_blocks   Block count; must be > 0.
 * @param timeout_ms   Transaction timeout in ms. Use `BLUSYS_TIMEOUT_FOREVER` to block.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, or `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_sdmmc_read_blocks(blusys_sdmmc_t *sdmmc,
                                      uint32_t start_block,
                                      void *buffer,
                                      uint32_t num_blocks,
                                      int timeout_ms);

/**
 * @brief Write @p num_blocks 512-byte blocks from @p buffer.
 *
 * @param sdmmc        Handle.
 * @param start_block  First block index.
 * @param buffer       Source buffer (`num_blocks * 512` bytes).
 * @param num_blocks   Block count; must be > 0.
 * @param timeout_ms   Transaction timeout in ms. Use `BLUSYS_TIMEOUT_FOREVER` to block.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, or `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_sdmmc_write_blocks(blusys_sdmmc_t *sdmmc,
                                       uint32_t start_block,
                                       const void *buffer,
                                       uint32_t num_blocks,
                                       int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
