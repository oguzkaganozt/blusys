/**
 * @file button.h
 * @brief Debounced push-button driver with press / release / long-press events.
 *
 * Built on top of `hal/gpio` with interrupt + software debouncing. Events are
 * delivered from a background task; the callback must not block. See
 * docs/hal/button.md.
 */

#ifndef BLUSYS_BUTTON_H
#define BLUSYS_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"
#include "blusys/hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open button. */
typedef struct blusys_button blusys_button_t;

/** @brief Button event kind. */
typedef enum {
    BLUSYS_BUTTON_EVENT_PRESS,        /**< Button transitioned to the pressed state (after debounce). */
    BLUSYS_BUTTON_EVENT_RELEASE,      /**< Button transitioned to the released state. */
    BLUSYS_BUTTON_EVENT_LONG_PRESS,   /**< Button held past `long_press_ms`. */
} blusys_button_event_t;

/**
 * @brief Callback fired from the button task when an event is detected.
 * @param button    Button that produced the event.
 * @param event     Event kind.
 * @param user_ctx  User pointer supplied to ::blusys_button_set_callback.
 */
typedef void (*blusys_button_callback_t)(blusys_button_t *button,
                                         blusys_button_event_t event,
                                         void *user_ctx);

/** @brief Configuration for ::blusys_button_open. */
typedef struct {
    int                pin;            /**< GPIO pin connected to the button. */
    blusys_gpio_pull_t pull_mode;      /**< GPIO pull; `BLUSYS_GPIO_PULL_UP` is recommended. */
    bool               active_low;     /**< `true` when the GPIO reads low while pressed (typical). */
    uint32_t           debounce_ms;    /**< Debounce window in ms; `0` selects 50 ms. */
    uint32_t           long_press_ms;  /**< Long-press threshold in ms; `0` disables long-press detection. */
} blusys_button_config_t;

/**
 * @brief Open a button and start its debounce task.
 * @param config      Button configuration.
 * @param out_button  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_IO` on GPIO setup failure.
 */
blusys_err_t blusys_button_open(const blusys_button_config_t *config,
                                 blusys_button_t **out_button);

/**
 * @brief Stop the debounce task and release the handle.
 * @param button  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p button is NULL.
 */
blusys_err_t blusys_button_close(blusys_button_t *button);

/**
 * @brief Register or replace the event callback.
 * @param button    Handle.
 * @param callback  Callback; NULL clears the existing registration.
 * @param user_ctx  Opaque pointer forwarded to @p callback.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p button is NULL.
 */
blusys_err_t blusys_button_set_callback(blusys_button_t *button,
                                         blusys_button_callback_t callback,
                                         void *user_ctx);

/**
 * @brief Sample the current pressed state (post-debounce).
 * @param button       Handle.
 * @param out_pressed  Output: `true` if currently pressed.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL.
 */
blusys_err_t blusys_button_read(blusys_button_t *button, bool *out_pressed);

#ifdef __cplusplus
}
#endif

#endif
