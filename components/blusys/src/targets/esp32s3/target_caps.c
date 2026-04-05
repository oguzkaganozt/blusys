#include "blusys_target_caps.h"

const blusys_target_caps_t *blusys_target_caps_get(void)
{
    static const blusys_target_caps_t caps = {
        .target = BLUSYS_TARGET_ESP32S3,
        .name = "ESP32-S3",
        .cpu_cores = 2,
        .feature_mask = BLUSYS_BASE_FEATURE_MASK | BLUSYS_PCNT_FEATURE_MASK | BLUSYS_TOUCH_FEATURE_MASK |
                        BLUSYS_SDMMC_FEATURE_MASK,
    };

    return &caps;
}
