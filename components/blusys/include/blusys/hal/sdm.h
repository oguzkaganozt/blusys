/**
 * @file sdm.h
 * @brief Sigma-delta modulator (SDM): 1-bit PDM output for low-cost DAC-like generation.
 *
 * Density value directly selects the modulator's 8-bit density setting. See
 * docs/hal/sdm.md.
 */

#ifndef BLUSYS_SDM_H
#define BLUSYS_SDM_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for an SDM channel. */
typedef struct blusys_sdm blusys_sdm_t;

/**
 * @brief Open an SDM channel on the given GPIO.
 * @param pin              GPIO number.
 * @param sample_rate_hz   Modulator sample rate in Hz.
 * @param out_sdm          Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.
 */
blusys_err_t blusys_sdm_open(int pin, uint32_t sample_rate_hz, blusys_sdm_t **out_sdm);

/** @brief Release the SDM channel and free its handle. */
blusys_err_t blusys_sdm_close(blusys_sdm_t *sdm);

/**
 * @brief Set the output density (-128 to 127).
 *
 * -128 = fully low, 0 = 50% duty, 127 = fully high. Drives the SDM's 8-bit
 * density register directly.
 *
 * @param sdm      Handle.
 * @param density  New density value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p sdm is NULL.
 */
blusys_err_t blusys_sdm_set_density(blusys_sdm_t *sdm, int8_t density);

#ifdef __cplusplus
}
#endif

#endif
