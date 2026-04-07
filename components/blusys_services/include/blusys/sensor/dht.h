#ifndef BLUSYS_DHT_H
#define BLUSYS_DHT_H

#include <stdbool.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque DHT sensor handle.
 */
typedef struct blusys_dht blusys_dht_t;

/**
 * @brief DHT sensor type.
 */
typedef enum {
    BLUSYS_DHT_TYPE_DHT11,      /**< DHT11: 0–50 °C (±2 °C), 20–80 % RH (±5 %) */
    BLUSYS_DHT_TYPE_DHT22,      /**< DHT22 / AM2302: −40–80 °C (±0.5 °C), 0–100 % RH (±2 %) */
} blusys_dht_type_t;

/**
 * @brief DHT sensor configuration.
 */
typedef struct {
    int              pin;   /**< GPIO pin connected to the DHT data line */
    blusys_dht_type_t type; /**< Sensor type (DHT11 or DHT22) */
} blusys_dht_config_t;

/**
 * @brief Temperature and humidity reading.
 */
typedef struct {
    float temperature_c;    /**< Temperature in degrees Celsius */
    float humidity_pct;     /**< Relative humidity in percent (0–100) */
} blusys_dht_reading_t;

/**
 * @brief Open a DHT sensor.
 *
 * Allocates resources and configures the RMT peripheral for bit-level
 * timing on the one-wire DHT bus.  An external 4.7 kΩ–10 kΩ pull-up
 * resistor on the data line is required.
 *
 * @param[in]  config     Sensor configuration.
 * @param[out] out_handle Handle written on success.
 * @return BLUSYS_OK, BLUSYS_ERR_INVALID_ARG, BLUSYS_ERR_NO_MEM
 */
blusys_err_t blusys_dht_open(const blusys_dht_config_t *config,
                              blusys_dht_t **out_handle);

/**
 * @brief Close a DHT sensor and release all resources.
 *
 * @param[in] handle Handle returned by blusys_dht_open().
 * @return BLUSYS_OK, BLUSYS_ERR_INVALID_ARG
 */
blusys_err_t blusys_dht_close(blusys_dht_t *handle);

/**
 * @brief Read temperature and humidity.
 *
 * The DHT sensor requires at least 2 s between consecutive reads
 * (1 s for DHT11).  If called too soon the function returns the
 * previous cached reading and sets @c *out_reading accordingly.
 *
 * @param[in]  handle      Handle returned by blusys_dht_open().
 * @param[out] out_reading Temperature and humidity written on success.
 * @return BLUSYS_OK, BLUSYS_ERR_INVALID_ARG, BLUSYS_ERR_IO (checksum / no response),
 *         BLUSYS_ERR_TIMEOUT
 */
blusys_err_t blusys_dht_read(blusys_dht_t *handle,
                              blusys_dht_reading_t *out_reading);

#ifdef __cplusplus
}
#endif

#endif
