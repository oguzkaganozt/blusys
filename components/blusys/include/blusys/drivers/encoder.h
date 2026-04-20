/**
 * @file encoder.h
 * @brief Quadrature rotary encoder driver with optional push-button.
 *
 * Uses PCNT (on ESP32/ESP32-S3) or a GPIO-interrupt fallback for step
 * counting, plus an optional debounced pushbutton. Callbacks run on a
 * driver task context — keep them short. See docs/hal/encoder.md.
 */

#ifndef BLUSYS_ENCODER_H
#define BLUSYS_ENCODER_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open encoder. */
typedef struct blusys_encoder blusys_encoder_t;

/** @brief Encoder event kind. */
typedef enum {
    BLUSYS_ENCODER_EVENT_CW,            /**< One detent clockwise. */
    BLUSYS_ENCODER_EVENT_CCW,           /**< One detent counter-clockwise. */
    BLUSYS_ENCODER_EVENT_PRESS,         /**< Push-button pressed (after debounce). */
    BLUSYS_ENCODER_EVENT_RELEASE,       /**< Push-button released. */
    BLUSYS_ENCODER_EVENT_LONG_PRESS,    /**< Push-button held past `long_press_ms`. */
} blusys_encoder_event_t;

/**
 * @brief Callback fired from the driver task for each encoder event.
 * @param encoder   Encoder that produced the event.
 * @param event     Event kind.
 * @param position  Current position after applying the event.
 * @param user_ctx  User pointer supplied to ::blusys_encoder_set_callback.
 */
typedef void (*blusys_encoder_callback_t)(blusys_encoder_t *encoder,
                                          blusys_encoder_event_t event,
                                          int position,
                                          void *user_ctx);

/** @brief Configuration for ::blusys_encoder_open. */
typedef struct {
    int      clk_pin;            /**< Encoder CLK (A) GPIO pin. */
    int      dt_pin;             /**< Encoder DT (B) GPIO pin. */
    int      sw_pin;             /**< Push-button GPIO pin; `-1` to disable the button. */
    uint32_t glitch_filter_ns;   /**< PCNT glitch filter width in ns (ESP32/S3 only); `0` disables. */
    uint32_t debounce_ms;        /**< Button debounce window in ms; `0` selects 50 ms. */
    uint32_t long_press_ms;      /**< Long-press threshold in ms; `0` disables long-press. */
    int      steps_per_detent;   /**< Quadrature pulses per mechanical detent; `0` selects 4. */
} blusys_encoder_config_t;

/**
 * @brief Open an encoder and start its driver task.
 * @param config       Configuration.
 * @param out_encoder  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`.
 */
blusys_err_t blusys_encoder_open(const blusys_encoder_config_t *config,
                                  blusys_encoder_t **out_encoder);

/**
 * @brief Stop the driver task and release the handle.
 * @param encoder  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p encoder is NULL.
 */
blusys_err_t blusys_encoder_close(blusys_encoder_t *encoder);

/**
 * @brief Register or replace the event callback.
 * @param encoder   Handle.
 * @param callback  Callback; NULL clears the existing registration.
 * @param user_ctx  Opaque pointer forwarded to @p callback.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p encoder is NULL.
 */
blusys_err_t blusys_encoder_set_callback(blusys_encoder_t *encoder,
                                          blusys_encoder_callback_t callback,
                                          void *user_ctx);

/**
 * @brief Read the current accumulated position (in detents).
 *
 * The position is signed; CW increments, CCW decrements. The value accumulates
 * from open time, or from the last ::blusys_encoder_reset_position call.
 *
 * @param encoder       Handle.
 * @param out_position  Output: current position.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL.
 */
blusys_err_t blusys_encoder_get_position(blusys_encoder_t *encoder,
                                          int *out_position);

/**
 * @brief Reset the accumulated position to zero.
 * @param encoder  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p encoder is NULL.
 */
blusys_err_t blusys_encoder_reset_position(blusys_encoder_t *encoder);

#ifdef __cplusplus
}
#endif

#endif
