#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define LED_STRIP_PIN       CONFIG_BLUSYS_LED_STRIP_PIN
#define LED_STRIP_LED_COUNT CONFIG_BLUSYS_LED_STRIP_LED_COUNT

void app_main(void)
{
    blusys_led_strip_t *strip = NULL;
    blusys_err_t err;
    uint32_t i;
    uint32_t step = 0u;

    printf("Blusys led_strip basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d  led_count: %d\n", LED_STRIP_PIN, LED_STRIP_LED_COUNT);
    printf("set pin and count via menuconfig if these defaults don't match your board\n");

    const blusys_led_strip_config_t config = {
        .pin       = LED_STRIP_PIN,
        .led_count = LED_STRIP_LED_COUNT,
    };

    err = blusys_led_strip_open(&config, &strip);
    if (err != BLUSYS_OK) {
        printf("blusys_led_strip_open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_led_strip_clear(strip, BLUSYS_TIMEOUT_FOREVER);
    if (err != BLUSYS_OK) {
        printf("blusys_led_strip_clear error: %s\n", blusys_err_string(err));
        blusys_led_strip_close(strip);
        return;
    }

    while (true) {
        /* Chase pattern: one lit pixel cycling R -> G -> B across the strip */
        uint8_t r = (step % 3u == 0u) ? 32u : 0u;
        uint8_t g = (step % 3u == 1u) ? 32u : 0u;
        uint8_t b = (step % 3u == 2u) ? 32u : 0u;
        uint32_t lit = (step / 3u) % (uint32_t) LED_STRIP_LED_COUNT;

        for (i = 0u; i < (uint32_t) LED_STRIP_LED_COUNT; i++) {
            uint8_t pr = (i == lit) ? r : 0u;
            uint8_t pg = (i == lit) ? g : 0u;
            uint8_t pb = (i == lit) ? b : 0u;
            blusys_led_strip_set_pixel(strip, i, pr, pg, pb);
        }

        err = blusys_led_strip_refresh(strip, BLUSYS_TIMEOUT_FOREVER);
        if (err != BLUSYS_OK) {
            printf("blusys_led_strip_refresh error: %s\n", blusys_err_string(err));
            break;
        }

        step++;
        vTaskDelay(pdMS_TO_TICKS(80));
    }

    blusys_led_strip_close(strip);
}
