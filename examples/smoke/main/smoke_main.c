#include <stdio.h>

#include "blusys/blusys.h"

static const char *feature_name(blusys_feature_t feature)
{
    switch (feature) {
    case BLUSYS_FEATURE_GPIO:
        return "gpio";
    case BLUSYS_FEATURE_UART:
        return "uart";
    case BLUSYS_FEATURE_I2C:
        return "i2c";
    case BLUSYS_FEATURE_SPI:
        return "spi";
    case BLUSYS_FEATURE_PWM:
        return "pwm";
    case BLUSYS_FEATURE_ADC:
        return "adc";
    case BLUSYS_FEATURE_TIMER:
        return "timer";
    default:
        return "unknown";
    }
}

void app_main(void)
{
    unsigned int feature;

    printf("Blusys smoke app\n");
    printf("version: %s\n", blusys_version_string());
    printf("version_packed: %u\n", (unsigned int) blusys_version_packed());
    printf("target: %s\n", blusys_target_name());
    printf("cpu_cores: %u\n", (unsigned int) blusys_target_cpu_cores());

    for (feature = 0; feature < BLUSYS_FEATURE_COUNT; ++feature) {
        printf("feature_%s: %s\n",
               feature_name((blusys_feature_t) feature),
               blusys_target_supports((blusys_feature_t) feature) ? "yes" : "no");
    }

    printf("status_ok_string: %s\n", blusys_err_string(BLUSYS_OK));
}
