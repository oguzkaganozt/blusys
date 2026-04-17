#if CONFIG_PLATFORM_LAB_SCENARIO_POWER_MGMT
#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys.h"


#define PM_MAX_FREQ_MHZ 80
#define PM_MIN_FREQ_MHZ 10

void run_platform_power_mgmt(void)
{
    blusys_err_t err;
    blusys_pm_lock_t *cpu_lock = NULL;
    blusys_pm_lock_t *sleep_lock = NULL;

    printf("target: %s\n", blusys_target_name());
    printf("power_mgmt supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_POWER_MGMT) ? "yes" : "no");

    blusys_pm_config_t cfg = {
        .max_freq_mhz      = PM_MAX_FREQ_MHZ,
        .min_freq_mhz      = PM_MIN_FREQ_MHZ,
        .light_sleep_enable = true,
    };
    err = blusys_pm_configure(&cfg);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_configure failed: %d\n", err);
        return;
    }
    printf("PM configured: max=%u MHz  min=%u MHz  light_sleep=true\n",
           PM_MAX_FREQ_MHZ, PM_MIN_FREQ_MHZ);

    blusys_pm_config_t readback = {0};
    err = blusys_pm_get_configuration(&readback);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_get_configuration failed: %d\n", err);
        return;
    }
    printf("PM readback:   max=%" PRIu32 " MHz  min=%" PRIu32 " MHz  light_sleep=%s\n",
           readback.max_freq_mhz, readback.min_freq_mhz,
           readback.light_sleep_enable ? "true" : "false");

    err = blusys_pm_lock_create(BLUSYS_PM_LOCK_CPU_FREQ_MAX, "cpu_max", &cpu_lock);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_lock_create(CPU_FREQ_MAX) failed: %d\n", err);
        return;
    }

    err = blusys_pm_lock_acquire(cpu_lock);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_lock_acquire(CPU_FREQ_MAX) failed: %d\n", err);
        blusys_pm_lock_delete(cpu_lock);
        return;
    }
    printf("CPU_FREQ_MAX lock held — CPU pinned at %u MHz for 2 s\n", PM_MAX_FREQ_MHZ);
    vTaskDelay(pdMS_TO_TICKS(2000));

    err = blusys_pm_lock_release(cpu_lock);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_lock_release(CPU_FREQ_MAX) failed: %d\n", err);
    }
    blusys_pm_lock_delete(cpu_lock);
    printf("CPU_FREQ_MAX lock released and deleted\n");

    err = blusys_pm_lock_create(BLUSYS_PM_LOCK_NO_LIGHT_SLEEP, "no_sleep", &sleep_lock);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_lock_create(NO_LIGHT_SLEEP) failed: %d\n", err);
        return;
    }

    err = blusys_pm_lock_acquire(sleep_lock);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_lock_acquire(NO_LIGHT_SLEEP) failed: %d\n", err);
        blusys_pm_lock_delete(sleep_lock);
        return;
    }
    printf("NO_LIGHT_SLEEP lock held — automatic light sleep suppressed for 1 s\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    err = blusys_pm_lock_release(sleep_lock);
    if (err != BLUSYS_OK) {
        printf("blusys_pm_lock_release(NO_LIGHT_SLEEP) failed: %d\n", err);
    }
    blusys_pm_lock_delete(sleep_lock);
    printf("NO_LIGHT_SLEEP lock released and deleted\n");

    printf("power_mgmt_basic: done\n");
}

#else
void run_platform_power_mgmt(void) {}
#endif /* CONFIG_PLATFORM_LAB_SCENARIO_POWER_MGMT */
