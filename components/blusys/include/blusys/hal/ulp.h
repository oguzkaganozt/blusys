/**
 * @file ulp.h
 * @brief Ultra-Low-Power coprocessor job runtime.
 *
 * Loads a prebuilt ULP FSM program that samples a GPIO at a configurable
 * period and wakes the main CPU from deep sleep when the level is stable for
 * a threshold number of samples. First release supports a single job kind;
 * the job list is expected to grow. See docs/hal/ulp.md.
 */

#ifndef BLUSYS_ULP_H
#define BLUSYS_ULP_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ULP job kind. */
typedef enum {
    BLUSYS_ULP_JOB_NONE = 0,       /**< No job configured. */
    BLUSYS_ULP_JOB_GPIO_WATCH,     /**< Sample a GPIO and wake on a stable level. */
} blusys_ulp_job_t;

/** @brief Configuration for ::BLUSYS_ULP_JOB_GPIO_WATCH. */
typedef struct {
    int      pin;             /**< RTC-capable GPIO to sample. */
    bool     wake_on_high;    /**< Wake when the stable level is high (`true`) or low (`false`). */
    uint32_t period_ms;       /**< Sampling period in milliseconds. */
    uint8_t  stable_samples;  /**< Consecutive samples required at the target level before waking. */
} blusys_ulp_gpio_watch_config_t;

/** @brief Snapshot of the current ULP job result. */
typedef struct {
    bool             valid;           /**< `true` if a job has run at least once since the last clear. */
    blusys_ulp_job_t job;             /**< Job kind that produced this result. */
    uint32_t         run_count;       /**< Number of ULP program iterations. */
    int32_t          last_value;      /**< Last sampled value (job-specific meaning). */
    bool             wake_triggered;  /**< `true` if the ULP triggered a wakeup since the last clear. */
} blusys_ulp_result_t;

/**
 * @brief Configure a GPIO-watch job. Does not start the ULP.
 * @param config  Job configuration.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL or out-of-range fields,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without a ULP FSM.
 */
blusys_err_t blusys_ulp_gpio_watch_configure(const blusys_ulp_gpio_watch_config_t *config);

/**
 * @brief Start the ULP program. Call after a `*_configure` step.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if no job is configured.
 */
blusys_err_t blusys_ulp_start(void);

/**
 * @brief Halt the ULP program if running.
 * @return `BLUSYS_OK` (idempotent — calling when stopped is not an error).
 */
blusys_err_t blusys_ulp_stop(void);

/** @brief Reset the cached result so a future ::blusys_ulp_get_result reports a fresh run. */
blusys_err_t blusys_ulp_clear_result(void);

/**
 * @brief Read the most recent ULP result.
 * @param out_result  Output pointer.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_result is NULL.
 */
blusys_err_t blusys_ulp_get_result(blusys_ulp_result_t *out_result);

#ifdef __cplusplus
}
#endif

#endif
