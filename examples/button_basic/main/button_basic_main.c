#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"

#define BUTTON_PIN           CONFIG_BLUSYS_BUTTON_PIN
#define BUTTON_LONG_PRESS_MS CONFIG_BLUSYS_BUTTON_LONG_PRESS_MS

static const char *event_name(blusys_button_event_t event)
{
    switch (event) {
    case BLUSYS_BUTTON_EVENT_PRESS:      return "PRESS";
    case BLUSYS_BUTTON_EVENT_RELEASE:    return "RELEASE";
    case BLUSYS_BUTTON_EVENT_LONG_PRESS: return "LONG_PRESS";
    default:                             return "UNKNOWN";
    }
}

static void on_button_event(blusys_button_t *button, blusys_button_event_t event, void *user_ctx)
{
    (void)button;
    (void)user_ctx;
    printf("button event: %s\n", event_name(event));
}

void app_main(void)
{
    blusys_button_t *button = NULL;
    blusys_err_t err;

    printf("Blusys button basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d  long_press_ms: %d\n", BUTTON_PIN, BUTTON_LONG_PRESS_MS);

    const blusys_button_config_t config = {
        .pin           = BUTTON_PIN,
        .pull_mode     = BLUSYS_GPIO_PULL_UP,
        .active_low    = true,
        .debounce_ms   = 50,
        .long_press_ms = BUTTON_LONG_PRESS_MS,
    };

    err = blusys_button_open(&config, &button);
    if (err != BLUSYS_OK) {
        printf("blusys_button_open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_button_set_callback(button, on_button_event, NULL);
    if (err != BLUSYS_OK) {
        printf("blusys_button_set_callback error: %s\n", blusys_err_string(err));
        blusys_button_close(button);
        return;
    }

    printf("waiting for button events on pin %d ...\n", BUTTON_PIN);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
