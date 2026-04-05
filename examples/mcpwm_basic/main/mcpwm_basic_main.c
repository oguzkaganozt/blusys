#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_MCPWM_BASIC_PIN_A     CONFIG_BLUSYS_MCPWM_BASIC_PIN_A
#define BLUSYS_MCPWM_BASIC_PIN_B     CONFIG_BLUSYS_MCPWM_BASIC_PIN_B
#define BLUSYS_MCPWM_BASIC_FREQ_HZ   20000u
#define BLUSYS_MCPWM_BASIC_DEAD_NS   500u
#define BLUSYS_MCPWM_BASIC_DUTY_STEP 50u

void app_main(void)
{
    blusys_mcpwm_t *mcpwm = NULL;
    blusys_err_t err;
    uint16_t duty;

    printf("Blusys mcpwm basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("mcpwm supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_MCPWM) ? "yes" : "no");

    if (!blusys_target_supports(BLUSYS_FEATURE_MCPWM)) {
        printf("mcpwm not supported on this target\n");
        return;
    }

    printf("pin_a: %d  pin_b: %d\n",
           BLUSYS_MCPWM_BASIC_PIN_A, BLUSYS_MCPWM_BASIC_PIN_B);

    err = blusys_mcpwm_open(BLUSYS_MCPWM_BASIC_PIN_A,
                            BLUSYS_MCPWM_BASIC_PIN_B,
                            BLUSYS_MCPWM_BASIC_FREQ_HZ,
                            0u,
                            BLUSYS_MCPWM_BASIC_DEAD_NS,
                            &mcpwm);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("observe pin_a and pin_b with an oscilloscope\n");

    /* Sweep duty 0 → 1000 → 0 */
    for (duty = 0u; duty <= 1000u; duty = (uint16_t) (duty + BLUSYS_MCPWM_BASIC_DUTY_STEP)) {
        err = blusys_mcpwm_set_duty(mcpwm, duty);
        if (err != BLUSYS_OK) {
            printf("set_duty error: %s\n", blusys_err_string(err));
            break;
        }
        printf("duty: %u\n", (unsigned int) duty);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    for (duty = 1000u; duty > 0u; duty = (uint16_t) (duty - BLUSYS_MCPWM_BASIC_DUTY_STEP)) {
        err = blusys_mcpwm_set_duty(mcpwm, duty);
        if (err != BLUSYS_OK) {
            printf("set_duty error: %s\n", blusys_err_string(err));
            break;
        }
        printf("duty: %u\n", (unsigned int) duty);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    blusys_mcpwm_close(mcpwm);
    printf("done\n");
}
