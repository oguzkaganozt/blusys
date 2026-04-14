#ifndef BLUSYS_BUTTON_H
#define BLUSYS_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"
#include "blusys/hal/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_button blusys_button_t;

typedef enum {
    BLUSYS_BUTTON_EVENT_PRESS,
    BLUSYS_BUTTON_EVENT_RELEASE,
    BLUSYS_BUTTON_EVENT_LONG_PRESS,
} blusys_button_event_t;

typedef void (*blusys_button_callback_t)(blusys_button_t *button,
                                         blusys_button_event_t event,
                                         void *user_ctx);

typedef struct {
    int                pin;
    blusys_gpio_pull_t pull_mode;     /* BLUSYS_GPIO_PULL_UP recommended */
    bool               active_low;    /* true = pressed when GPIO is low (typical) */
    uint32_t           debounce_ms;   /* 0 → defaults to 50 ms */
    uint32_t           long_press_ms; /* 0 → long-press detection disabled */
} blusys_button_config_t;

blusys_err_t blusys_button_open(const blusys_button_config_t *config,
                                 blusys_button_t **out_button);
blusys_err_t blusys_button_close(blusys_button_t *button);
blusys_err_t blusys_button_set_callback(blusys_button_t *button,
                                         blusys_button_callback_t callback,
                                         void *user_ctx);
blusys_err_t blusys_button_read(blusys_button_t *button, bool *out_pressed);

#ifdef __cplusplus
}
#endif

#endif
