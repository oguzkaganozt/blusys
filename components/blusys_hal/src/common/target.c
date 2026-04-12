#include "blusys/target.h"

#include "sdkconfig.h"

#include "blusys/internal/blusys_target_caps.h"

blusys_target_t blusys_target_get(void)
{
    return blusys_target_caps_get()->target;
}

const char *blusys_target_name(void)
{
    return blusys_target_caps_get()->name;
}

uint8_t blusys_target_cpu_cores(void)
{
    return blusys_target_caps_get()->cpu_cores;
}

bool blusys_target_supports(blusys_feature_t feature)
{
    const blusys_target_caps_t *caps = blusys_target_caps_get();

    if ((unsigned) feature >= BLUSYS_FEATURE_COUNT) {
        return false;
    }

    bool supported = (caps->feature_mask & (1ull << (unsigned) feature)) != 0;

#if !defined(CONFIG_BT_NIMBLE_ENABLED)
    if (feature == BLUSYS_FEATURE_BLUETOOTH || feature == BLUSYS_FEATURE_BLE_GATT) {
        return false;
    }
#endif

    return supported;
}
