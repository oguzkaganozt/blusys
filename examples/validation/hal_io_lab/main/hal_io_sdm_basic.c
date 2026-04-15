#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_SDM_BASIC

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "blusys/blusys.h"

#define BLUSYS_SDM_BASIC_PIN            CONFIG_BLUSYS_SDM_BASIC_PIN
#define BLUSYS_SDM_BASIC_SAMPLE_RATE_HZ CONFIG_BLUSYS_SDM_BASIC_SAMPLE_RATE_HZ
#define BLUSYS_SDM_BASIC_DENSITY_STEP   10

void run_sdm_basic(void)
{
    blusys_sdm_t *sdm = NULL;
    blusys_err_t err;
    int8_t density;

    printf("Blusys sdm basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("sdm supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_SDM) ? "yes" : "no");

    if (!blusys_target_supports(BLUSYS_FEATURE_SDM)) {
        printf("sdm not supported on this target\n");
        return;
    }

    printf("pin: %d  sample_rate_hz: %u\n",
           BLUSYS_SDM_BASIC_PIN, (unsigned int) BLUSYS_SDM_BASIC_SAMPLE_RATE_HZ);

    err = blusys_sdm_open(BLUSYS_SDM_BASIC_PIN,
                          BLUSYS_SDM_BASIC_SAMPLE_RATE_HZ,
                          &sdm);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("connect an RC low-pass filter (e.g. 1k + 100nF) to measure analog output\n");

    /* Sweep density -90 → +90 → -90 */
    for (density = -90; density <= 90; density = (int8_t) (density + BLUSYS_SDM_BASIC_DENSITY_STEP)) {
        err = blusys_sdm_set_density(sdm, density);
        if (err != BLUSYS_OK) {
            printf("set_density error: %s\n", blusys_err_string(err));
            break;
        }
        printf("density: %d\n", (int) density);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for (density = 90; density >= -90; density = (int8_t) (density - BLUSYS_SDM_BASIC_DENSITY_STEP)) {
        err = blusys_sdm_set_density(sdm, density);
        if (err != BLUSYS_OK) {
            printf("set_density error: %s\n", blusys_err_string(err));
            break;
        }
        printf("density: %d\n", (int) density);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    blusys_sdm_close(sdm);
    printf("done\n");
}


#else

void run_sdm_basic(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_SDM_BASIC */
