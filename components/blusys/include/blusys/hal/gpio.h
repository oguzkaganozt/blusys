#ifndef BLUSYS_GPIO_H
#define BLUSYS_GPIO_H

#include <stdbool.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_GPIO_PULL_NONE = 0,
    BLUSYS_GPIO_PULL_UP,
    BLUSYS_GPIO_PULL_DOWN,
    BLUSYS_GPIO_PULL_UP_DOWN,
} blusys_gpio_pull_t;

typedef enum {
    BLUSYS_GPIO_INTERRUPT_DISABLED = 0,
    BLUSYS_GPIO_INTERRUPT_RISING,
    BLUSYS_GPIO_INTERRUPT_FALLING,
    BLUSYS_GPIO_INTERRUPT_ANY_EDGE,
    BLUSYS_GPIO_INTERRUPT_LOW_LEVEL,
    BLUSYS_GPIO_INTERRUPT_HIGH_LEVEL,
} blusys_gpio_interrupt_mode_t;

typedef bool (*blusys_gpio_callback_t)(int pin, void *user_ctx);

blusys_err_t blusys_gpio_reset(int pin);
blusys_err_t blusys_gpio_set_input(int pin);
blusys_err_t blusys_gpio_set_output(int pin);
blusys_err_t blusys_gpio_set_pull_mode(int pin, blusys_gpio_pull_t pull);
blusys_err_t blusys_gpio_read(int pin, bool *out_level);
blusys_err_t blusys_gpio_write(int pin, bool level);
blusys_err_t blusys_gpio_set_interrupt(int pin, blusys_gpio_interrupt_mode_t mode);
blusys_err_t blusys_gpio_set_callback(int pin, blusys_gpio_callback_t callback, void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
