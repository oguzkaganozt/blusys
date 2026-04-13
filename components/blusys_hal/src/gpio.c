#include "blusys/gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/internal/blusys_esp_err.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    blusys_gpio_callback_t callback;
    void *user_ctx;
    blusys_gpio_interrupt_mode_t mode;
    unsigned int callback_active;
    bool handler_installed;
} blusys_gpio_interrupt_entry_t;

static blusys_gpio_interrupt_entry_t blusys_gpio_interrupt_entries[GPIO_PIN_COUNT];
static portMUX_TYPE blusys_gpio_interrupt_lock = portMUX_INITIALIZER_UNLOCKED;
static bool blusys_gpio_isr_service_installed;

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

static bool blusys_gpio_translate_interrupt_mode(blusys_gpio_interrupt_mode_t mode, gpio_int_type_t *out_mode)
{
    if (out_mode == NULL) {
        return false;
    }

    switch (mode) {
    case BLUSYS_GPIO_INTERRUPT_DISABLED:
        *out_mode = GPIO_INTR_DISABLE;
        return true;
    case BLUSYS_GPIO_INTERRUPT_RISING:
        *out_mode = GPIO_INTR_POSEDGE;
        return true;
    case BLUSYS_GPIO_INTERRUPT_FALLING:
        *out_mode = GPIO_INTR_NEGEDGE;
        return true;
    case BLUSYS_GPIO_INTERRUPT_ANY_EDGE:
        *out_mode = GPIO_INTR_ANYEDGE;
        return true;
    case BLUSYS_GPIO_INTERRUPT_LOW_LEVEL:
        *out_mode = GPIO_INTR_LOW_LEVEL;
        return true;
    case BLUSYS_GPIO_INTERRUPT_HIGH_LEVEL:
        *out_mode = GPIO_INTR_HIGH_LEVEL;
        return true;
    default:
        return false;
    }
}

static bool blusys_gpio_callback_active(int pin)
{
    bool active;

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    active = blusys_gpio_interrupt_entries[pin].callback_active > 0u;
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    return active;
}

static void blusys_gpio_wait_for_callback_idle(int pin)
{
    while (blusys_gpio_callback_active(pin)) {
        taskYIELD();
    }
}

static blusys_err_t blusys_gpio_ensure_isr_service(void)
{
    esp_err_t esp_err;

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    if (blusys_gpio_isr_service_installed) {
        portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);
        return BLUSYS_OK;
    }
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    esp_err = gpio_install_isr_service(0);
    if ((esp_err != ESP_OK) && (esp_err != ESP_ERR_INVALID_STATE)) {
        return blusys_translate_esp_err(esp_err);
    }

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    blusys_gpio_isr_service_installed = true;
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    return BLUSYS_OK;
}

static void blusys_gpio_clear_interrupt_state(int pin)
{
    blusys_gpio_interrupt_entry_t *entry = &blusys_gpio_interrupt_entries[pin];

    entry->callback = NULL;
    entry->user_ctx = NULL;
    entry->mode = BLUSYS_GPIO_INTERRUPT_DISABLED;
    entry->handler_installed = false;
}

static void blusys_gpio_isr_handler(void *arg)
{
    int pin = (int) (intptr_t) arg;
    blusys_gpio_callback_t callback;
    void *user_ctx;
    bool should_yield = false;

    portENTER_CRITICAL_ISR(&blusys_gpio_interrupt_lock);
    blusys_gpio_interrupt_entries[pin].callback_active += 1u;
    callback = blusys_gpio_interrupt_entries[pin].callback;
    user_ctx = blusys_gpio_interrupt_entries[pin].user_ctx;
    portEXIT_CRITICAL_ISR(&blusys_gpio_interrupt_lock);

    if (callback != NULL) {
        should_yield = callback(pin, user_ctx);
    }

    portENTER_CRITICAL_ISR(&blusys_gpio_interrupt_lock);
    blusys_gpio_interrupt_entries[pin].callback_active -= 1u;
    portEXIT_CRITICAL_ISR(&blusys_gpio_interrupt_lock);

    if (should_yield) {
        portYIELD_FROM_ISR();
    }
}

blusys_err_t blusys_gpio_reset(int pin)
{
    esp_err_t esp_err;

    if (!blusys_gpio_is_valid_input_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err = gpio_intr_disable((gpio_num_t) pin);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    if (blusys_gpio_isr_service_installed) {
        esp_err = gpio_isr_handler_remove((gpio_num_t) pin);
        if ((esp_err != ESP_OK) && (esp_err != ESP_ERR_INVALID_STATE)) {
            return blusys_translate_esp_err(esp_err);
        }
    }

    blusys_gpio_wait_for_callback_idle(pin);

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    blusys_gpio_clear_interrupt_state(pin);
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

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

blusys_err_t blusys_gpio_set_interrupt(int pin, blusys_gpio_interrupt_mode_t mode)
{
    gpio_int_type_t intr_type;
    esp_err_t esp_err;
    bool handler_installed;

    if (!blusys_gpio_is_valid_input_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!blusys_gpio_translate_interrupt_mode(mode, &intr_type)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err = gpio_set_intr_type((gpio_num_t) pin, intr_type);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    esp_err = gpio_input_enable((gpio_num_t) pin);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    handler_installed = blusys_gpio_interrupt_entries[pin].handler_installed;
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    esp_err = ((mode == BLUSYS_GPIO_INTERRUPT_DISABLED) || !handler_installed) ?
                  gpio_intr_disable((gpio_num_t) pin) :
                  gpio_intr_enable((gpio_num_t) pin);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    blusys_gpio_interrupt_entries[pin].mode = mode;
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_set_callback(int pin, blusys_gpio_callback_t callback, void *user_ctx)
{
    blusys_gpio_interrupt_entry_t *entry;
    blusys_err_t err;
    esp_err_t esp_err;
    bool added_handler = false;
    bool handler_installed;
    bool wait_for_idle = false;
    blusys_gpio_interrupt_mode_t mode;

    if (!blusys_gpio_is_valid_input_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    entry = &blusys_gpio_interrupt_entries[pin];
    handler_installed = entry->handler_installed;
    mode = entry->mode;
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    if (callback != NULL) {
        err = blusys_gpio_ensure_isr_service();
        if (err != BLUSYS_OK) {
            return err;
        }

        if (mode == BLUSYS_GPIO_INTERRUPT_DISABLED) {
            return BLUSYS_ERR_INVALID_STATE;
        }

        if (!handler_installed) {
            esp_err = gpio_isr_handler_add((gpio_num_t) pin,
                                           blusys_gpio_isr_handler,
                                           (void *) (intptr_t) pin);
            if (esp_err != ESP_OK) {
                return blusys_translate_esp_err(esp_err);
            }

            portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
            entry->handler_installed = true;
            portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);
            added_handler = true;
        }

        portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
        if ((entry->callback != NULL) && ((entry->callback != callback) || (entry->user_ctx != user_ctx))) {
            entry->callback = NULL;
            entry->user_ctx = NULL;
            wait_for_idle = true;
        } else if (entry->callback == callback) {
            entry->user_ctx = user_ctx;
        }
        portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

        if (wait_for_idle) {
            blusys_gpio_wait_for_callback_idle(pin);

            portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
            entry->callback = callback;
            entry->user_ctx = user_ctx;
            portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);
        }

        if (!wait_for_idle) {
            portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
            entry->callback = callback;
            entry->user_ctx = user_ctx;
            portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);
        }

        esp_err = gpio_intr_enable((gpio_num_t) pin);
        if (esp_err != ESP_OK) {
            if (added_handler) {
                gpio_isr_handler_remove((gpio_num_t) pin);
                portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
                entry->handler_installed = false;
                entry->callback = NULL;
                entry->user_ctx = NULL;
                portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);
            }
            return blusys_translate_esp_err(esp_err);
        }

        return BLUSYS_OK;
    }

    esp_err = gpio_intr_disable((gpio_num_t) pin);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    if (handler_installed) {
        esp_err = gpio_isr_handler_remove((gpio_num_t) pin);
        if (esp_err != ESP_OK) {
            return blusys_translate_esp_err(esp_err);
        }
    }

    blusys_gpio_wait_for_callback_idle(pin);

    portENTER_CRITICAL(&blusys_gpio_interrupt_lock);
    entry->callback = NULL;
    entry->user_ctx = NULL;
    entry->handler_installed = false;
    portEXIT_CRITICAL(&blusys_gpio_interrupt_lock);

    return BLUSYS_OK;
}
