#include "sdkconfig.h"

#if CONFIG_REF_HAL_REFERENCE_SCENARIO_ADC_BASIC

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "blusys/blusys.h"

#define BLUSYS_ADC_BASIC_PIN CONFIG_BLUSYS_ADC_BASIC_PIN

#if CONFIG_BLUSYS_ADC_BASIC_ATTEN_DB_12
#define BLUSYS_ADC_BASIC_ATTEN BLUSYS_ADC_ATTEN_DB_12
#define BLUSYS_ADC_BASIC_ATTEN_NAME "12 dB"
#else
#define BLUSYS_ADC_BASIC_ATTEN BLUSYS_ADC_ATTEN_DB_0
#define BLUSYS_ADC_BASIC_ATTEN_NAME "0 dB"
#endif

void run_adc_basic(void)
{
    blusys_adc_t *adc = NULL;
    blusys_err_t err;

    printf("Blusys adc basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d\n", BLUSYS_ADC_BASIC_PIN);
    printf("attenuation: %s\n", BLUSYS_ADC_BASIC_ATTEN_NAME);
    printf("connect the analog signal to an ADC1-capable GPIO and adjust the example pin in menuconfig if needed\n");

    err = blusys_adc_open(BLUSYS_ADC_BASIC_PIN, BLUSYS_ADC_BASIC_ATTEN, &adc);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    while (true) {
        int raw = 0;
        int mv = 0;

        err = blusys_adc_read_raw(adc, &raw);
        if (err != BLUSYS_OK) {
            printf("read raw error: %s\n", blusys_err_string(err));
            break;
        }

        err = blusys_adc_read_mv(adc, &mv);
        if (err == BLUSYS_OK) {
            printf("raw=%d mv=%d\n", raw, mv);
        } else if (err == BLUSYS_ERR_NOT_SUPPORTED) {
            printf("raw=%d mv=unavailable\n", raw);
        } else {
            printf("read mv error: %s\n", blusys_err_string(err));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    err = blusys_adc_close(adc);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}


#else

void run_adc_basic(void) {}

#endif /* CONFIG_REF_HAL_REFERENCE_SCENARIO_ADC_BASIC */
