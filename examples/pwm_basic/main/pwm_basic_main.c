#include <stdio.h>

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_PWM_BASIC_PIN CONFIG_BLUSYS_PWM_BASIC_PIN

#define BLUSYS_PWM_BASIC_FREQ_HZ 1000u

void app_main(void)
{
    static const uint16_t duty_steps[] = {100u, 500u, 900u};
    blusys_pwm_t *pwm = NULL;
    blusys_err_t err;
    size_t step_index = 0u;

    printf("Blusys pwm basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d\n", BLUSYS_PWM_BASIC_PIN);
    printf("set a board-safe PWM pin in menuconfig if this default does not match your board\n");

    err = blusys_pwm_open(BLUSYS_PWM_BASIC_PIN, BLUSYS_PWM_BASIC_FREQ_HZ, duty_steps[0], &pwm);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_pwm_start(pwm);
    if (err != BLUSYS_OK) {
        printf("start error: %s\n", blusys_err_string(err));
        blusys_pwm_close(pwm);
        return;
    }

    while (true) {
        uint16_t duty_permille = duty_steps[step_index];

        err = blusys_pwm_set_duty(pwm, duty_permille);
        if (err != BLUSYS_OK) {
            printf("set duty error: %s\n", blusys_err_string(err));
            break;
        }

        printf("freq_hz=%u duty_permille=%u\n",
               (unsigned int) BLUSYS_PWM_BASIC_FREQ_HZ,
               (unsigned int) duty_permille);

        step_index = (step_index + 1u) % (sizeof(duty_steps) / sizeof(duty_steps[0]));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    blusys_pwm_close(pwm);
}
