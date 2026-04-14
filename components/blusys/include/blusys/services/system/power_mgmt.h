#ifndef BLUSYS_POWER_MGMT_H
#define BLUSYS_POWER_MGMT_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t max_freq_mhz;
    uint32_t min_freq_mhz;
    bool     light_sleep_enable;
} blusys_pm_config_t;

typedef enum {
    BLUSYS_PM_LOCK_CPU_FREQ_MAX,
    BLUSYS_PM_LOCK_APB_FREQ_MAX,
    BLUSYS_PM_LOCK_NO_LIGHT_SLEEP,
} blusys_pm_lock_type_t;

typedef struct blusys_pm_lock blusys_pm_lock_t;

blusys_err_t blusys_pm_configure(const blusys_pm_config_t *config);
blusys_err_t blusys_pm_get_configuration(blusys_pm_config_t *out_config);
blusys_err_t blusys_pm_lock_create(blusys_pm_lock_type_t type, const char *name,
                                    blusys_pm_lock_t **out_lock);
blusys_err_t blusys_pm_lock_delete(blusys_pm_lock_t *lock);
blusys_err_t blusys_pm_lock_acquire(blusys_pm_lock_t *lock);
blusys_err_t blusys_pm_lock_release(blusys_pm_lock_t *lock);

#ifdef __cplusplus
}
#endif

#endif
