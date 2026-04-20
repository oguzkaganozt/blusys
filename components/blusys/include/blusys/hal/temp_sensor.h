/**
 * @file temp_sensor.h
 * @brief On-chip internal temperature sensor (approximate die temperature).
 *
 * Meant for rough die-temperature observability, not calibrated measurement.
 * See docs/hal/temp_sensor.md for accuracy caveats.
 */

#ifndef BLUSYS_TEMP_SENSOR_H
#define BLUSYS_TEMP_SENSOR_H

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for the on-chip temperature sensor. */
typedef struct blusys_temp_sensor blusys_temp_sensor_t;

/**
 * @brief Enable the temperature sensor.
 * @param out_tsens  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_tsens is NULL,
 *         translated ESP-IDF error on startup failure.
 */
blusys_err_t blusys_temp_sensor_open(blusys_temp_sensor_t **out_tsens);

/** @brief Disable the sensor and free its handle. */
blusys_err_t blusys_temp_sensor_close(blusys_temp_sensor_t *tsens);

/**
 * @brief Read the current temperature in degrees Celsius.
 * @param tsens        Handle.
 * @param out_celsius  Destination for the reading.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_celsius is NULL.
 */
blusys_err_t blusys_temp_sensor_read_celsius(blusys_temp_sensor_t *tsens, float *out_celsius);

#ifdef __cplusplus
}
#endif

#endif
