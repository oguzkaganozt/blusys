#include "blusys/gpio.h"

#include "blusys_esp_err.h"

#include "driver/gpio.h"

static bool blusys_gpio_is_valid_input_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

static bool blusys_gpio_is_valid_output_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_gpio_translate_pull_mode(blusys_gpio_pull_t pull, gpio_pull_mode_t *out_pull)
{
    if (out_pull == NULL) {
        return false;
    }

    switch (pull) {
    case BLUSYS_GPIO_PULL_NONE:
        *out_pull = GPIO_FLOATING;
        return true;
    case BLUSYS_GPIO_PULL_UP:
        *out_pull = GPIO_PULLUP_ONLY;
        return true;
    case BLUSYS_GPIO_PULL_DOWN:
        *out_pull = GPIO_PULLDOWN_ONLY;
        return true;
    case BLUSYS_GPIO_PULL_UP_DOWN:
        *out_pull = GPIO_PULLUP_PULLDOWN;
        return true;
    default:
        return false;
    }
}

blusys_err_t blusys_gpio_reset(int pin)
{
    if (!blusys_gpio_is_valid_input_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return blusys_translate_esp_err(gpio_reset_pin((gpio_num_t) pin));
}

blusys_err_t blusys_gpio_set_input(int pin)
{
    if (!blusys_gpio_is_valid_input_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return blusys_translate_esp_err(gpio_set_direction((gpio_num_t) pin, GPIO_MODE_INPUT));
}

blusys_err_t blusys_gpio_set_output(int pin)
{
    if (!blusys_gpio_is_valid_output_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return blusys_translate_esp_err(gpio_set_direction((gpio_num_t) pin, GPIO_MODE_OUTPUT));
}

blusys_err_t blusys_gpio_set_pull_mode(int pin, blusys_gpio_pull_t pull)
{
    gpio_pull_mode_t pull_mode;

    if (!blusys_gpio_is_valid_input_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!blusys_gpio_translate_pull_mode(pull, &pull_mode)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return blusys_translate_esp_err(gpio_set_pull_mode((gpio_num_t) pin, pull_mode));
}

blusys_err_t blusys_gpio_read(int pin, bool *out_level)
{
    if ((!blusys_gpio_is_valid_input_pin(pin)) || (out_level == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_level = gpio_get_level((gpio_num_t) pin) != 0;

    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_write(int pin, bool level)
{
    if (!blusys_gpio_is_valid_output_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return blusys_translate_esp_err(gpio_set_level((gpio_num_t) pin, level ? 1u : 0u));
}
