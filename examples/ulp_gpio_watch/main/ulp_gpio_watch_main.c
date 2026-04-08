#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

#define BLUSYS_ULP_GPIO_WATCH_PERIOD_MS      50
#define BLUSYS_ULP_GPIO_WATCH_STABLE_SAMPLES 3
#define BLUSYS_ULP_GPIO_WATCH_HOLD_MS        \
    (BLUSYS_ULP_GPIO_WATCH_PERIOD_MS * BLUSYS_ULP_GPIO_WATCH_STABLE_SAMPLES)

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
#define BLUSYS_ULP_GPIO_WATCH_PIN 4
#else
#define BLUSYS_ULP_GPIO_WATCH_PIN 0
#endif

static const char *wakeup_cause_name(blusys_sleep_wakeup_t cause)
{
    switch (cause) {
        case BLUSYS_SLEEP_WAKEUP_TIMER:    return "timer";
        case BLUSYS_SLEEP_WAKEUP_EXT0:     return "ext0";
        case BLUSYS_SLEEP_WAKEUP_EXT1:     return "ext1";
        case BLUSYS_SLEEP_WAKEUP_TOUCHPAD: return "touchpad";
        case BLUSYS_SLEEP_WAKEUP_GPIO:     return "gpio";
        case BLUSYS_SLEEP_WAKEUP_UART:     return "uart";
        case BLUSYS_SLEEP_WAKEUP_ULP:      return "ulp";
        default:                           return "undefined";
    }
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Blusys ULP GPIO watch\n");
    printf("target: %s\n", blusys_target_name());

    if (!blusys_target_supports(BLUSYS_FEATURE_ULP)) {
        printf("ULP is not supported on this target\n");
        return;
    }

    blusys_sleep_wakeup_t cause = blusys_sleep_get_wakeup_cause();
    printf("wake cause: %s\n", wakeup_cause_name(cause));

    blusys_ulp_result_t result;
    blusys_err_t err = blusys_ulp_get_result(&result);
    if (err == BLUSYS_OK && result.valid) {
        printf("ulp result | job=%d run_count=%lu last_value=%ld wake_triggered=%d\n",
               (int) result.job,
               (unsigned long) result.run_count,
               (long) result.last_value,
               result.wake_triggered ? 1 : 0);
    }

    err = blusys_ulp_gpio_watch_configure(&(blusys_ulp_gpio_watch_config_t) {
        .pin = BLUSYS_ULP_GPIO_WATCH_PIN,
        .wake_on_high = false,
        .period_ms = BLUSYS_ULP_GPIO_WATCH_PERIOD_MS,
        .stable_samples = BLUSYS_ULP_GPIO_WATCH_STABLE_SAMPLES,
    });
    if (err != BLUSYS_OK) {
        printf("ulp_gpio_watch_configure error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_ulp_start();
    if (err != BLUSYS_OK) {
        printf("ulp_start error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_sleep_enable_ulp_wakeup();
    if (err != BLUSYS_OK) {
        printf("enable_ulp_wakeup error: %s\n", blusys_err_string(err));
        return;
    }

    printf("watching RTC GPIO %d for a stable low level\n", BLUSYS_ULP_GPIO_WATCH_PIN);
    printf("sampling every %d ms, %d consecutive matches required\n",
           BLUSYS_ULP_GPIO_WATCH_PERIOD_MS,
           BLUSYS_ULP_GPIO_WATCH_STABLE_SAMPLES);
    printf("wire the pin with an external pull-up and hold it low for at least %d ms to wake\n",
           BLUSYS_ULP_GPIO_WATCH_HOLD_MS);
    printf("entering deep sleep...\n");

    blusys_sleep_enter_deep();
}
