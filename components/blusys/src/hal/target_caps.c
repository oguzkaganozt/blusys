#include "blusys/hal/internal/target_caps.h"

#ifdef __has_include
#  if __has_include("sdkconfig.h")
#    include "sdkconfig.h"
#  endif
#endif

const blusys_target_caps_t *blusys_target_caps_get(void)
{
#if defined(CONFIG_IDF_TARGET_ESP32)
    static const blusys_target_caps_t caps = {
        .target = BLUSYS_TARGET_ESP32,
        .name = "ESP32",
        .cpu_cores = 2,
        .feature_mask = BLUSYS_BASE_FEATURE_MASK | BLUSYS_PCNT_FEATURE_MASK | BLUSYS_TOUCH_FEATURE_MASK |
                        BLUSYS_DAC_FEATURE_MASK | BLUSYS_SDMMC_FEATURE_MASK | BLUSYS_MCPWM_FEATURE_MASK |
                        BLUSYS_ULP_FEATURE_MASK,
    };
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    static const blusys_target_caps_t caps = {
        .target = BLUSYS_TARGET_ESP32S3,
        .name = "ESP32-S3",
        .cpu_cores = 2,
        .feature_mask = BLUSYS_BASE_FEATURE_MASK | BLUSYS_PCNT_FEATURE_MASK | BLUSYS_TOUCH_FEATURE_MASK |
                        BLUSYS_SDMMC_FEATURE_MASK | BLUSYS_TEMP_SENSOR_FEATURE_MASK | BLUSYS_MCPWM_FEATURE_MASK |
                        BLUSYS_USB_HOST_FEATURE_MASK | BLUSYS_USB_DEVICE_FEATURE_MASK |
                        BLUSYS_ULP_FEATURE_MASK,
    };
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    static const blusys_target_caps_t caps = {
        .target = BLUSYS_TARGET_ESP32C3,
        .name = "ESP32-C3",
        .cpu_cores = 1,
        .feature_mask = BLUSYS_BASE_FEATURE_MASK | BLUSYS_TEMP_SENSOR_FEATURE_MASK,
    };
#else
#  error "blusys currently supports only esp32, esp32c3, and esp32s3"
#endif
    return &caps;
}
