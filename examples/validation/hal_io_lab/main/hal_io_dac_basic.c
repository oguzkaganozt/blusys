#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_DAC_BASIC

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "blusys/blusys.h"

#define BLUSYS_DAC_BASIC_PIN CONFIG_BLUSYS_DAC_BASIC_PIN
#define BLUSYS_DAC_BASIC_STEP 16u

void run_dac_basic(void)
{
    blusys_dac_t *dac = NULL;
    blusys_err_t err;
    uint8_t value = 0u;
    bool rising = true;

    printf("Blusys dac basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("dac supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_DAC) ? "yes" : "no");
    printf("pin: %d\n", BLUSYS_DAC_BASIC_PIN);

    err = blusys_dac_open(BLUSYS_DAC_BASIC_PIN, &dac);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("observe the DAC pin with a multimeter or oscilloscope\n");

    while (true) {
        err = blusys_dac_write(dac, value);
        if (err != BLUSYS_OK) {
            printf("write error: %s\n", blusys_err_string(err));
            break;
        }

        printf("dac_value: %u\n", (unsigned int) value);

        if (rising) {
            if (value >= (uint8_t) (255u - BLUSYS_DAC_BASIC_STEP)) {
                value = 255u;
                rising = false;
            } else {
                value = (uint8_t) (value + BLUSYS_DAC_BASIC_STEP);
            }
        } else if (value <= BLUSYS_DAC_BASIC_STEP) {
            value = 0u;
            rising = true;
        } else {
            value = (uint8_t) (value - BLUSYS_DAC_BASIC_STEP);
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }

    blusys_dac_close(dac);
}


#else

void run_dac_basic(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_DAC_BASIC */
