/**
 * @file power_mgmt.h
 * @brief ESP-IDF dynamic frequency scaling (DFS) and power-management locks.
 *
 * Configure min/max CPU frequency and optional automatic light sleep, then
 * acquire named locks to prevent the power manager from dropping below the
 * required state while a task is working. See docs/services/power_mgmt.md.
 */

#ifndef BLUSYS_POWER_MGMT_H
#define BLUSYS_POWER_MGMT_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Power-management configuration applied by ::blusys_pm_configure. */
typedef struct {
    uint32_t max_freq_mhz;        /**< Maximum CPU frequency in MHz. */
    uint32_t min_freq_mhz;        /**< Minimum CPU frequency in MHz. */
    bool     light_sleep_enable;  /**< Enable automatic light sleep while idle. */
} blusys_pm_config_t;

/** @brief Power-management lock kind. */
typedef enum {
    BLUSYS_PM_LOCK_CPU_FREQ_MAX,     /**< Holds the CPU at max frequency while acquired. */
    BLUSYS_PM_LOCK_APB_FREQ_MAX,     /**< Holds the APB bus at max frequency while acquired. */
    BLUSYS_PM_LOCK_NO_LIGHT_SLEEP,   /**< Prevents automatic light sleep while acquired. */
} blusys_pm_lock_type_t;

/** @brief Opaque handle to a power-management lock. */
typedef struct blusys_pm_lock blusys_pm_lock_t;

/**
 * @brief Apply a power-management configuration.
 *
 * Subsequent calls replace the previous configuration. Individual tasks
 * can override the chosen frequency or sleep behaviour via locks.
 *
 * @param config  Configuration.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, or a translated ESP-IDF error.
 */
blusys_err_t blusys_pm_configure(const blusys_pm_config_t *config);

/**
 * @brief Read the active power-management configuration.
 * @param out_config  Output.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_config is NULL.
 */
blusys_err_t blusys_pm_get_configuration(blusys_pm_config_t *out_config);

/**
 * @brief Create a named power-management lock.
 * @param type      Lock kind.
 * @param name      Short name for diagnostics.
 * @param out_lock  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, or `BLUSYS_ERR_NO_MEM`.
 */
blusys_err_t blusys_pm_lock_create(blusys_pm_lock_type_t type, const char *name,
                                    blusys_pm_lock_t **out_lock);

/**
 * @brief Release and free a lock created by ::blusys_pm_lock_create.
 *
 * Must not be called while the lock is held.
 *
 * @param lock  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE`.
 */
blusys_err_t blusys_pm_lock_delete(blusys_pm_lock_t *lock);

/**
 * @brief Acquire the lock. Nests reference-counted.
 * @param lock  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_pm_lock_acquire(blusys_pm_lock_t *lock);

/**
 * @brief Release a previously acquired lock.
 * @param lock  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_pm_lock_release(blusys_pm_lock_t *lock);

#ifdef __cplusplus
}
#endif

#endif
