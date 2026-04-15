#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_WDT_BASIC

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

#define BLUSYS_WDT_BASIC_NUM_FEEDS 10
#define BLUSYS_WDT_BASIC_TIMEOUT_MS 5000
#define BLUSYS_WDT_BASIC_DELAY_MS   1000

void run_wdt_basic(void)
{
    blusys_err_t err;

    printf("Blusys wdt basic\n");
    printf("target: %s\n", blusys_target_name());

    err = blusys_wdt_init(BLUSYS_WDT_BASIC_TIMEOUT_MS, true);
    if (err != BLUSYS_OK) {
        printf("wdt_init error: %s\n", blusys_err_string(err));
        return;
    }
    printf("wdt_init: ok\n");

    err = blusys_wdt_subscribe();
    if (err != BLUSYS_OK) {
        printf("wdt_subscribe error: %s\n", blusys_err_string(err));
        blusys_wdt_deinit();
        return;
    }
    printf("wdt_subscribe: ok\n");

    for (int i = 1; i <= BLUSYS_WDT_BASIC_NUM_FEEDS; i++) {
        err = blusys_wdt_feed();
        if (err != BLUSYS_OK) {
            printf("wdt_feed error: %s\n", blusys_err_string(err));
            blusys_wdt_unsubscribe();
            blusys_wdt_deinit();
            return;
        }
        printf("feeding %d/%d\n", i, BLUSYS_WDT_BASIC_NUM_FEEDS);
        vTaskDelay(BLUSYS_WDT_BASIC_DELAY_MS / portTICK_PERIOD_MS);
    }

    err = blusys_wdt_unsubscribe();
    if (err != BLUSYS_OK) {
        printf("wdt_unsubscribe error: %s\n", blusys_err_string(err));
        blusys_wdt_deinit();
        return;
    }
    printf("wdt_unsubscribe: ok\n");

    err = blusys_wdt_deinit();
    if (err != BLUSYS_OK) {
        printf("wdt_deinit error: %s\n", blusys_err_string(err));
        return;
    }
    printf("wdt_deinit: ok\n");
}


#else

void run_wdt_basic(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_WDT_BASIC */
