/**
 * @file dac.h
 * @brief On-chip 8-bit DAC: write a voltage level to a DAC-capable GPIO.
 *
 * ESP32 only (GPIO25 / GPIO26). On ESP32-C3 and ESP32-S3 every call returns
 * `BLUSYS_ERR_NOT_SUPPORTED`; use `blusys_target_supports(BLUSYS_FEATURE_DAC)`
 * to check at runtime. See docs/hal/dac.md for the mental model.
 */

#ifndef BLUSYS_DAC_H
#define BLUSYS_DAC_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a DAC channel. */
typedef struct blusys_dac blusys_dac_t;

/**
 * @brief Open a DAC channel on a DAC-capable GPIO.
 *
 * Valid pins on ESP32: GPIO25 (channel 1), GPIO26 (channel 2).
 *
 * @param pin      DAC-capable GPIO number.
 * @param out_dac  Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid pins,
 *         `BLUSYS_ERR_BUSY` if the channel is already open,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on unsupported targets.
 */
blusys_err_t blusys_dac_open(int pin, blusys_dac_t **out_dac);

/**
 * @brief Release a DAC channel and free its handle.
 * @param dac Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p dac is NULL.
 */
blusys_err_t blusys_dac_close(blusys_dac_t *dac);

/**
 * @brief Drive the DAC output.
 *
 * The output voltage is approximately `value / 255 × VDD`.
 *
 * @param dac    Handle.
 * @param value  8-bit output value (0 = 0V, 255 = VDD).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if close has started.
 */
blusys_err_t blusys_dac_write(blusys_dac_t *dac, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
