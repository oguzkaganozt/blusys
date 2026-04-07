#include "blusys/system/power_mgmt.h"

#include "blusys/internal/blusys_esp_err.h"

#ifdef CONFIG_PM_ENABLE

#include <stdlib.h>

#include "esp_pm.h"

struct blusys_pm_lock {
    esp_pm_lock_handle_t handle;
};

static bool translate_lock_type(blusys_pm_lock_type_t type, esp_pm_lock_type_t *out)
{
    switch (type) {
        case BLUSYS_PM_LOCK_CPU_FREQ_MAX:   *out = ESP_PM_CPU_FREQ_MAX;   return true;
        case BLUSYS_PM_LOCK_APB_FREQ_MAX:   *out = ESP_PM_APB_FREQ_MAX;   return true;
        case BLUSYS_PM_LOCK_NO_LIGHT_SLEEP: *out = ESP_PM_NO_LIGHT_SLEEP; return true;
        default:                                                           return false;
    }
}

blusys_err_t blusys_pm_configure(const blusys_pm_config_t *config)
{
    if (config == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_pm_config_t esp_cfg = {
        /* uint32_t cast to int is safe: max real-world value is 240 MHz */
        .max_freq_mhz      = (int) config->max_freq_mhz,
        .min_freq_mhz      = (int) config->min_freq_mhz,
        .light_sleep_enable = config->light_sleep_enable,
    };

    return blusys_translate_esp_err(esp_pm_configure(&esp_cfg));
}

blusys_err_t blusys_pm_get_configuration(blusys_pm_config_t *out_config)
{
    esp_pm_config_t esp_cfg;
    esp_err_t esp_err;

    if (out_config == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err = esp_pm_get_configuration(&esp_cfg);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    out_config->max_freq_mhz      = (uint32_t) esp_cfg.max_freq_mhz;
    out_config->min_freq_mhz      = (uint32_t) esp_cfg.min_freq_mhz;
    out_config->light_sleep_enable = esp_cfg.light_sleep_enable;
    return BLUSYS_OK;
}

blusys_err_t blusys_pm_lock_create(blusys_pm_lock_type_t type, const char *name,
                                    blusys_pm_lock_t **out_lock)
{
    blusys_pm_lock_t *lock;
    esp_pm_lock_type_t esp_type;
    esp_err_t esp_err;

    if (out_lock == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (!translate_lock_type(type, &esp_type)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    lock = calloc(1, sizeof(*lock));
    if (lock == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    esp_err = esp_pm_lock_create(esp_type, 0, name, &lock->handle);
    if (esp_err != ESP_OK) {
        free(lock);
        return blusys_translate_esp_err(esp_err);
    }

    *out_lock = lock;
    return BLUSYS_OK;
}

blusys_err_t blusys_pm_lock_delete(blusys_pm_lock_t *lock)
{
    esp_err_t esp_err;

    if (lock == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err = esp_pm_lock_delete(lock->handle);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    free(lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_pm_lock_acquire(blusys_pm_lock_t *lock)
{
    if (lock == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    return blusys_translate_esp_err(esp_pm_lock_acquire(lock->handle));
}

blusys_err_t blusys_pm_lock_release(blusys_pm_lock_t *lock)
{
    if (lock == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    return blusys_translate_esp_err(esp_pm_lock_release(lock->handle));
}

#else /* !CONFIG_PM_ENABLE */

blusys_err_t blusys_pm_configure(const blusys_pm_config_t *config)
{
    (void) config;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pm_get_configuration(blusys_pm_config_t *out_config)
{
    (void) out_config;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pm_lock_create(blusys_pm_lock_type_t type, const char *name,
                                    blusys_pm_lock_t **out_lock)
{
    (void) type;
    (void) name;
    (void) out_lock;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pm_lock_delete(blusys_pm_lock_t *lock)
{
    (void) lock;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pm_lock_acquire(blusys_pm_lock_t *lock)
{
    (void) lock;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pm_lock_release(blusys_pm_lock_t *lock)
{
    (void) lock;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* CONFIG_PM_ENABLE */
