#include <stdio.h>

#include "blusys/blusys.h"

#define BLUSYS_SLEEP_BASIC_ITERATIONS   3
#define BLUSYS_SLEEP_BASIC_WAKEUP_US    (5 * 1000 * 1000)

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
        default:                            return "undefined";
    }
}

void app_main(void)
{
    blusys_err_t err;

    printf("Blusys sleep basic\n");
    printf("target: %s\n", blusys_target_name());

    for (int i = 0; i < BLUSYS_SLEEP_BASIC_ITERATIONS; i++) {
        printf("wakeup cause: %s\n", wakeup_cause_name(blusys_sleep_get_wakeup_cause()));

        err = blusys_sleep_enable_timer_wakeup(BLUSYS_SLEEP_BASIC_WAKEUP_US);
        if (err != BLUSYS_OK) {
            printf("enable_timer_wakeup error: %s\n", blusys_err_string(err));
            return;
        }

        printf("entering light sleep for 5s...\n");

        err = blusys_sleep_enter_light();
        if (err != BLUSYS_OK) {
            printf("enter_light error: %s\n", blusys_err_string(err));
            return;
        }
    }

    printf("done\n");
}
