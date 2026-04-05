#include "blusys_target_caps.h"

const blusys_target_caps_t *blusys_target_caps_get(void)
{
    static const blusys_target_caps_t caps = {
        .target = BLUSYS_TARGET_ESP32C3,
        .name = "ESP32-C3",
        .cpu_cores = 1,
        .feature_mask = BLUSYS_BASE_FEATURE_MASK | BLUSYS_TEMP_SENSOR_FEATURE_MASK,
    };

    return &caps;
}
