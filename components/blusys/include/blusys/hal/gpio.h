/**
 * @file gpio.h
 * @brief Pin-based digital IO: input, output, pull resistors, and direct ISR callbacks.
 *
 * See docs/hal/gpio.md for thread-safety, ISR rules, and board caveats.
 */

#ifndef BLUSYS_GPIO_H
#define BLUSYS_GPIO_H

#include <stdbool.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Internal pull-resistor configuration for a GPIO pin. */
typedef enum {
    BLUSYS_GPIO_PULL_NONE = 0,   /**< No internal pull resistor. */
    BLUSYS_GPIO_PULL_UP,         /**< Internal pull-up enabled. */
    BLUSYS_GPIO_PULL_DOWN,       /**< Internal pull-down enabled. */
    BLUSYS_GPIO_PULL_UP_DOWN,    /**< Both pull-up and pull-down enabled. */
} blusys_gpio_pull_t;

/** @brief Interrupt trigger mode for a GPIO pin. */
typedef enum {
    BLUSYS_GPIO_INTERRUPT_DISABLED = 0,  /**< Interrupt disabled. */
    BLUSYS_GPIO_INTERRUPT_RISING,        /**< Trigger on rising edge. */
    BLUSYS_GPIO_INTERRUPT_FALLING,       /**< Trigger on falling edge. */
    BLUSYS_GPIO_INTERRUPT_ANY_EDGE,      /**< Trigger on both edges. */
    BLUSYS_GPIO_INTERRUPT_LOW_LEVEL,     /**< Trigger while held low. */
    BLUSYS_GPIO_INTERRUPT_HIGH_LEVEL,    /**< Trigger while held high. */
} blusys_gpio_interrupt_mode_t;

/**
 * @brief Callback invoked directly in GPIO ISR context.
 * @param pin       The GPIO that triggered the interrupt.
 * @param user_ctx  User context pointer provided at registration.
 * @return `true` to request a FreeRTOS yield after waking a higher-priority task.
 * @warning Runs in ISR context. Must not block or call blocking Blusys APIs.
 */
typedef bool (*blusys_gpio_callback_t)(int pin, void *user_ctx);

/**
 * @brief Reset a pin to its default state (input, no pull, no interrupt, no callback).
 * @param pin GPIO number.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_reset(int pin);

/**
 * @brief Configure the pin as a digital input.
 * @param pin GPIO number.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_set_input(int pin);

/**
 * @brief Configure the pin as a digital output.
 * @param pin GPIO number.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_set_output(int pin);

/**
 * @brief Set the internal pull-resistor configuration for a pin.
 * @param pin  GPIO number.
 * @param pull Pull mode.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_set_pull_mode(int pin, blusys_gpio_pull_t pull);

/**
 * @brief Read the current digital level of a pin.
 * @param pin       GPIO number.
 * @param out_level Destination: `true` for high, `false` for low.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_read(int pin, bool *out_level);

/**
 * @brief Drive the output level of a pin.
 * @param pin   GPIO number.
 * @param level `true` for high, `false` for low.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_write(int pin, bool level);

/**
 * @brief Configure the interrupt trigger mode for a pin.
 *
 * Interrupt delivery is armed only when both a non-disabled mode is set here
 * and a callback is registered with ::blusys_gpio_set_callback.
 *
 * @param pin  GPIO number.
 * @param mode Trigger mode; use `BLUSYS_GPIO_INTERRUPT_DISABLED` to disable.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_gpio_set_interrupt(int pin, blusys_gpio_interrupt_mode_t mode);

/**
 * @brief Register an ISR-context callback for GPIO interrupts on a pin.
 * @param pin      GPIO number.
 * @param callback ISR-context callback, or NULL to clear.
 * @param user_ctx Opaque pointer passed back to the callback unchanged.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid pins,
 *         `BLUSYS_ERR_INVALID_STATE` if a non-disabled interrupt mode is not
 *         configured first.
 */
blusys_err_t blusys_gpio_set_callback(int pin, blusys_gpio_callback_t callback, void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
