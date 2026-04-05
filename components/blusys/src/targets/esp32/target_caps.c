#include "blusys_target_caps.h"

const blusys_target_caps_t *blusys_target_caps_get(void)
{
    static const blusys_target_caps_t caps = {
        .target = BLUSYS_TARGET_ESP32,
        .name = "ESP32",
        .cpu_cores = 2,
        .feature_mask = BLUSYS_IMPLEMENTED_FEATURE_MASK,
    };

    return &caps;
}
