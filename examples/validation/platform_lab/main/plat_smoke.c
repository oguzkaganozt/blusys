#if CONFIG_PLATFORM_LAB_SCENARIO_SMOKE
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
    case BLUSYS_FEATURE_PCNT:
        return "pcnt";
    case BLUSYS_FEATURE_RMT:
        return "rmt";
    case BLUSYS_FEATURE_TWAI:
        return "twai";
    default:
        return "unknown";
    }
}

void run_platform_smoke(void)
{
    unsigned int feature;

    printf("### run_platform_smoke ENTERED ###\n");
    fflush(stdout);

    printf("Blusys smoke app\n");
    fflush(stdout);
    printf("version: %s\n", blusys_version_string());
    fflush(stdout);
    printf("version_packed: %u\n", (unsigned int) blusys_version_packed());
    fflush(stdout);
    printf("target: %s\n", blusys_target_name());
    fflush(stdout);
    printf("cpu_cores: %u\n", (unsigned int) blusys_target_cpu_cores());
    fflush(stdout);

    for (feature = 0; feature < BLUSYS_FEATURE_COUNT; ++feature) {
        printf("feature_%s: %s\n",
               feature_name((blusys_feature_t) feature),
               blusys_target_supports((blusys_feature_t) feature) ? "yes" : "no");
        fflush(stdout);
    }

    printf("status_ok_string: %s\n", blusys_err_string(BLUSYS_OK));
    fflush(stdout);
}

#else
void run_platform_smoke(void) {}
#endif /* CONFIG_PLATFORM_LAB_SCENARIO_SMOKE */
