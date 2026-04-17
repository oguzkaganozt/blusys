#include "blusys/drivers/button.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "blusys/hal/internal/callback_lifecycle.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/gpio.h"

#define BUTTON_DEFAULT_DEBOUNCE_MS 50u
#define BUTTON_DOMAIN err_domain_driver_button

struct blusys_button {
    int                        pin;
    bool                       active_low;
    uint32_t                   debounce_ms;
    uint32_t                   long_press_ms;
    blusys_lock_t              lock;
    portMUX_TYPE               state_lock;     /* protects callback_active, pressed, closing */
    esp_timer_handle_t         debounce_timer;
    esp_timer_handle_t         long_press_timer; /* NULL when long_press_ms == 0 */
    blusys_button_callback_t   callback;
    void                      *user_ctx;
    volatile unsigned          callback_active;
    bool                       pressed;        /* debounced logical state */
    bool                       closing;
};

static void invoke_callback(blusys_button_t *btn, blusys_button_event_t event)
{
    blusys_button_callback_t cb;
    void *ctx;

    portENTER_CRITICAL(&btn->state_lock);
    cb  = btn->callback;
    ctx = btn->user_ctx;
    if ((cb == NULL) || !blusys_callback_lifecycle_try_enter(&btn->callback_active, &btn->closing)) {
        portEXIT_CRITICAL(&btn->state_lock);
        return;
    }
    portEXIT_CRITICAL(&btn->state_lock);

    cb(btn, event, ctx);

    portENTER_CRITICAL(&btn->state_lock);
    blusys_callback_lifecycle_leave(&btn->callback_active);
    portEXIT_CRITICAL(&btn->state_lock);
}

static void debounce_timer_cb(void *arg)
{
    blusys_button_t *btn = arg;
    bool raw_level;
    bool new_pressed;

    portENTER_CRITICAL(&btn->state_lock);
    if (blusys_callback_lifecycle_is_closing(&btn->closing)) {
        portEXIT_CRITICAL(&btn->state_lock);
        return;
    }
    portEXIT_CRITICAL(&btn->state_lock);

    if (blusys_gpio_read(btn->pin, &raw_level) != BLUSYS_OK) {
        return;
    }

    new_pressed = btn->active_low ? !raw_level : raw_level;

    portENTER_CRITICAL(&btn->state_lock);
    if (new_pressed == btn->pressed) {
        portEXIT_CRITICAL(&btn->state_lock);
        return;
    }
    btn->pressed = new_pressed;
    portEXIT_CRITICAL(&btn->state_lock);

    if (new_pressed) {
        if (btn->long_press_timer != NULL) {
            esp_timer_stop(btn->long_press_timer);
            esp_timer_start_once(btn->long_press_timer, (uint64_t)btn->long_press_ms * 1000u);
        }
        invoke_callback(btn, BLUSYS_BUTTON_EVENT_PRESS);
    } else {
        if (btn->long_press_timer != NULL) {
            esp_timer_stop(btn->long_press_timer);
        }
        invoke_callback(btn, BLUSYS_BUTTON_EVENT_RELEASE);
    }
}

static void long_press_timer_cb(void *arg)
{
    blusys_button_t *btn = arg;

    portENTER_CRITICAL(&btn->state_lock);
    if (blusys_callback_lifecycle_is_closing(&btn->closing) || !btn->pressed) {
        portEXIT_CRITICAL(&btn->state_lock);
        return;
    }
    portEXIT_CRITICAL(&btn->state_lock);

    invoke_callback(btn, BLUSYS_BUTTON_EVENT_LONG_PRESS);
}

static bool IRAM_ATTR button_gpio_isr_cb(int pin, void *user_ctx)
{
    blusys_button_t *btn = user_ctx;

    (void)pin;

    esp_timer_stop(btn->debounce_timer);
    esp_timer_start_once(btn->debounce_timer, (uint64_t)btn->debounce_ms * 1000u);

    return false;
}

blusys_err_t blusys_button_open(const blusys_button_config_t *config,
                                 blusys_button_t **out_button)
{
    blusys_button_t *btn;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((config == NULL) || (out_button == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    btn = calloc(1, sizeof(*btn));
    if (btn == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    btn->pin           = config->pin;
    btn->active_low    = config->active_low;
    btn->debounce_ms   = (config->debounce_ms > 0u) ? config->debounce_ms : BUTTON_DEFAULT_DEBOUNCE_MS;
    btn->long_press_ms = config->long_press_ms;
    btn->state_lock    = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    err = blusys_lock_init(&btn->lock);
    if (err != BLUSYS_OK) {
        free(btn);
        return err;
    }

    const esp_timer_create_args_t debounce_args = {
        .callback        = debounce_timer_cb,
        .arg             = btn,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "btn_debounce",
    };
    esp_err = esp_timer_create(&debounce_args, &btn->debounce_timer);
    if (esp_err != ESP_OK) {
        blusys_lock_deinit(&btn->lock);
        free(btn);
        return blusys_translate_esp_err_in(BUTTON_DOMAIN, esp_err);
    }

    if (btn->long_press_ms > 0u) {
        const esp_timer_create_args_t long_press_args = {
            .callback        = long_press_timer_cb,
            .arg             = btn,
            .dispatch_method = ESP_TIMER_TASK,
            .name            = "btn_longpress",
        };
        esp_err = esp_timer_create(&long_press_args, &btn->long_press_timer);
        if (esp_err != ESP_OK) {
            esp_timer_delete(btn->debounce_timer);
            blusys_lock_deinit(&btn->lock);
            free(btn);
            return blusys_translate_esp_err_in(BUTTON_DOMAIN, esp_err);
        }
    }

    err = blusys_gpio_reset(btn->pin);
    if (err != BLUSYS_OK) {
        goto fail_gpio;
    }

    err = blusys_gpio_set_input(btn->pin);
    if (err != BLUSYS_OK) {
        goto fail_gpio;
    }

    err = blusys_gpio_set_pull_mode(btn->pin, config->pull_mode);
    if (err != BLUSYS_OK) {
        goto fail_gpio;
    }

    err = blusys_gpio_set_interrupt(btn->pin, BLUSYS_GPIO_INTERRUPT_ANY_EDGE);
    if (err != BLUSYS_OK) {
        goto fail_gpio;
    }

    err = blusys_gpio_set_callback(btn->pin, button_gpio_isr_cb, btn);
    if (err != BLUSYS_OK) {
        blusys_gpio_set_interrupt(btn->pin, BLUSYS_GPIO_INTERRUPT_DISABLED);
        goto fail_gpio;
    }

    *out_button = btn;
    return BLUSYS_OK;

fail_gpio:
    if (btn->long_press_timer != NULL) {
        esp_timer_delete(btn->long_press_timer);
    }
    esp_timer_delete(btn->debounce_timer);
    blusys_lock_deinit(&btn->lock);
    free(btn);
    return err;
}

blusys_err_t blusys_button_close(blusys_button_t *button)
{
    if (button == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_gpio_set_callback(button->pin, NULL, NULL);

    portENTER_CRITICAL(&button->state_lock);
    blusys_callback_lifecycle_set_closing(&button->closing, true);
    portEXIT_CRITICAL(&button->state_lock);

    esp_timer_stop(button->debounce_timer);
    if (button->long_press_timer != NULL) {
        esp_timer_stop(button->long_press_timer);
    }

    blusys_callback_lifecycle_wait_for_idle(&button->callback_active);

    esp_timer_delete(button->debounce_timer);
    if (button->long_press_timer != NULL) {
        esp_timer_delete(button->long_press_timer);
    }

    blusys_lock_deinit(&button->lock);
    free(button);
    return BLUSYS_OK;
}

blusys_err_t blusys_button_set_callback(blusys_button_t *button,
                                         blusys_button_callback_t callback,
                                         void *user_ctx)
{
    blusys_err_t err;

    if (button == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&button->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&button->state_lock);
    if (blusys_callback_lifecycle_is_closing(&button->closing)) {
        portEXIT_CRITICAL(&button->state_lock);
        blusys_lock_give(&button->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    button->callback  = callback;
    button->user_ctx  = user_ctx;
    portEXIT_CRITICAL(&button->state_lock);

    blusys_callback_lifecycle_wait_for_idle(&button->callback_active);

    blusys_lock_give(&button->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_button_read(blusys_button_t *button, bool *out_pressed)
{
    blusys_err_t err;
    bool raw_level;

    if ((button == NULL) || (out_pressed == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_gpio_read(button->pin, &raw_level);
    if (err != BLUSYS_OK) {
        return err;
    }

    *out_pressed = button->active_low ? !raw_level : raw_level;
    return BLUSYS_OK;
}
