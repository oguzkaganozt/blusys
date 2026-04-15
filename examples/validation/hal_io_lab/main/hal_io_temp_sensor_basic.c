#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_TEMP_SENSOR_BASIC

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

#define BLUSYS_TEMP_SENSOR_BASIC_NUM_READINGS 5
#define BLUSYS_TEMP_SENSOR_BASIC_DELAY_MS     1000

void run_temp_sensor_basic(void)
{
    blusys_temp_sensor_t *tsens = NULL;
    blusys_err_t err;
    float celsius;

    printf("Blusys temp_sensor basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("temp_sensor supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_TEMP_SENSOR) ? "yes" : "no");

    err = blusys_temp_sensor_open(&tsens);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("temp_sensor_open: ok\n");

    for (int i = 1; i <= BLUSYS_TEMP_SENSOR_BASIC_NUM_READINGS; i++) {
        err = blusys_temp_sensor_read_celsius(tsens, &celsius);
        if (err != BLUSYS_OK) {
            printf("read_celsius error: %s\n", blusys_err_string(err));
            blusys_temp_sensor_close(tsens);
            return;
        }
        printf("reading %d: %.1f C\n", i, celsius);
        vTaskDelay(BLUSYS_TEMP_SENSOR_BASIC_DELAY_MS / portTICK_PERIOD_MS);
    }

    err = blusys_temp_sensor_close(tsens);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
        return;
    }
    printf("temp_sensor_close: ok\n");
}


#else

void run_temp_sensor_basic(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_TEMP_SENSOR_BASIC */
