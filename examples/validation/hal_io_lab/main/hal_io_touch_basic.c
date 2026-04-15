#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_TOUCH_BASIC

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "blusys/blusys.h"

#define BLUSYS_TOUCH_BASIC_PIN CONFIG_BLUSYS_TOUCH_BASIC_PIN

void run_touch_basic(void)
{
    blusys_touch_t *touch = NULL;
    blusys_err_t err;
    uint32_t value;

    printf("Blusys touch basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("touch supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_TOUCH) ? "yes" : "no");
    printf("pin: %d\n", BLUSYS_TOUCH_BASIC_PIN);

    err = blusys_touch_open(BLUSYS_TOUCH_BASIC_PIN, &touch);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("touch the configured pad and watch the filtered reading change\n");

    while (true) {
        err = blusys_touch_read(touch, &value);
        if (err != BLUSYS_OK) {
            printf("read error: %s\n", blusys_err_string(err));
            break;
        }

        printf("touch_value: %" PRIu32 "\n", value);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    blusys_touch_close(touch);
}


#else

void run_touch_basic(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_TOUCH_BASIC */
