#include <stdio.h>

#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_RMT_BASIC_PIN CONFIG_BLUSYS_RMT_BASIC_PIN

static const blusys_rmt_pulse_t blusys_rmt_basic_waveform[] = {
    {.level = true, .duration_us = 1000u},
    {.level = false, .duration_us = 1000u},
    {.level = true, .duration_us = 500u},
    {.level = false, .duration_us = 500u},
    {.level = true, .duration_us = 250u},
    {.level = false, .duration_us = 250u},
};

void app_main(void)
{
    blusys_rmt_t *rmt = NULL;
    blusys_err_t err;

    printf("Blusys rmt basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d\n", BLUSYS_RMT_BASIC_PIN);
    printf("set a board-safe output pin in menuconfig if this default does not match your board\n");

    err = blusys_rmt_open(BLUSYS_RMT_BASIC_PIN, false, &rmt);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    while (true) {
        err = blusys_rmt_write(rmt,
                               blusys_rmt_basic_waveform,
                               sizeof(blusys_rmt_basic_waveform) / sizeof(blusys_rmt_basic_waveform[0]),
                               BLUSYS_TIMEOUT_FOREVER);
        if (err != BLUSYS_OK) {
            printf("write error: %s\n", blusys_err_string(err));
            break;
        }

        printf("sent %u pulses\n",
               (unsigned int) (sizeof(blusys_rmt_basic_waveform) / sizeof(blusys_rmt_basic_waveform[0])));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    blusys_rmt_close(rmt);
}
