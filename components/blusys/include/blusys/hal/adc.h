/**
 * @file adc.h
 * @brief Single-channel ADC input: raw conversion values and calibrated millivolt readings.
 *
 * See docs/hal/adc.md for board caveats, thread-safety rules, and limits.
 */

#ifndef BLUSYS_ADC_H
#define BLUSYS_ADC_H

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a single ADC channel. */
typedef struct blusys_adc blusys_adc_t;

/** @brief Input attenuation and full-scale voltage range for an ADC channel. */
typedef enum {
    BLUSYS_ADC_ATTEN_DB_0   = 0, /**< 0–750 mV full-scale. */
    BLUSYS_ADC_ATTEN_DB_2_5,     /**< 0–1050 mV full-scale. */
    BLUSYS_ADC_ATTEN_DB_6,       /**< 0–1300 mV full-scale. */
    BLUSYS_ADC_ATTEN_DB_12,      /**< 0–2500 mV full-scale. */
} blusys_adc_atten_t;

/**
 * @brief Open one ADC channel on a GPIO pin.
 *
 * ADC unit and channel are resolved internally from @p pin.
 *
 * @param pin      GPIO number; must be ADC1-capable on the target.
 * @param atten    Input attenuation / voltage range.
 * @param out_adc  Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid pin or attenuation,
 *         `BLUSYS_ERR_BUSY` if the channel is already owned.
 */
blusys_err_t blusys_adc_open(int pin, blusys_adc_atten_t atten, blusys_adc_t **out_adc);

/**
 * @brief Release an ADC channel and free its handle.
 * @param adc Handle from ::blusys_adc_open.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p adc is NULL.
 */
blusys_err_t blusys_adc_close(blusys_adc_t *adc);

/**
 * @brief Read the raw ADC conversion value.
 *
 * Range depends on hardware resolution (typically 0–4095 for 12-bit).
 *
 * @param adc      Handle.
 * @param out_raw  Destination for the raw value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_raw is NULL.
 */
blusys_err_t blusys_adc_read_raw(blusys_adc_t *adc, int *out_raw);

/**
 * @brief Read a calibrated millivolt value from the ADC channel.
 *
 * Requires factory calibration data stored in eFuse or OTP and a supported
 * attenuation mode.
 *
 * @param adc     Handle.
 * @param out_mv  Destination for the millivolt value.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` if calibrated conversion is unavailable,
 *         `BLUSYS_ERR_INVALID_ARG` if @p out_mv is NULL.
 */
blusys_err_t blusys_adc_read_mv(blusys_adc_t *adc, int *out_mv);

#ifdef __cplusplus
}
#endif

#endif
