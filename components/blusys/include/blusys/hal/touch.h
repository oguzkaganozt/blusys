/**
 * @file touch.h
 * @brief Capacitive touch sensor: filtered touch readings from a touch-capable GPIO.
 *
 * Only one touch handle may be open across the whole process. Not supported
 * on ESP32-C3 (every call returns `BLUSYS_ERR_NOT_SUPPORTED`). See
 * docs/hal/touch.md.
 */

#ifndef BLUSYS_TOUCH_H
#define BLUSYS_TOUCH_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for one touch GPIO. */
typedef struct blusys_touch blusys_touch_t;

/**
 * @brief Enable the touch controller and begin background scanning on @p pin.
 *
 * Only one touch handle may be open at a time.
 *
 * @param pin        Touch-capable GPIO (target-dependent).
 * @param out_touch  Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for non-touch GPIOs or NULL pointer,
 *         `BLUSYS_ERR_BUSY` if another touch handle is already open,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on ESP32-C3.
 */
blusys_err_t blusys_touch_open(int pin, blusys_touch_t **out_touch);

/** @brief Stop background scanning and free the handle. */
blusys_err_t blusys_touch_close(blusys_touch_t *touch);

/**
 * @brief Read the current filtered touch value.
 *
 * A higher value typically indicates stronger touch contact, but the absolute
 * scale is target- and board-dependent.
 *
 * @param touch      Handle.
 * @param out_value  Output pointer for the touch reading.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_STATE` if called after close has started,
 *         `BLUSYS_ERR_INVALID_ARG` if @p out_value is NULL.
 */
blusys_err_t blusys_touch_read(blusys_touch_t *touch, uint32_t *out_value);

#ifdef __cplusplus
}
#endif

#endif
