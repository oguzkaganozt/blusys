#ifndef BLUSYS_GPIO_H
#define BLUSYS_GPIO_H

#include <stdbool.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_GPIO_PULL_NONE = 0,
    BLUSYS_GPIO_PULL_UP,
    BLUSYS_GPIO_PULL_DOWN,
    BLUSYS_GPIO_PULL_UP_DOWN,
} blusys_gpio_pull_t;

blusys_err_t blusys_gpio_reset(int pin);
blusys_err_t blusys_gpio_set_input(int pin);
blusys_err_t blusys_gpio_set_output(int pin);
blusys_err_t blusys_gpio_set_pull_mode(int pin, blusys_gpio_pull_t pull);
blusys_err_t blusys_gpio_read(int pin, bool *out_level);
blusys_err_t blusys_gpio_write(int pin, bool level);

#ifdef __cplusplus
}
#endif

#endif
