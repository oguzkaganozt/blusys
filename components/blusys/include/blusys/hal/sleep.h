/**
 * @file sleep.h
 * @brief Light and deep sleep with configurable wakeup sources.
 *
 * Light sleep halts the CPU and retains RAM; deep sleep retains only the RTC
 * domain and restarts execution from `app_main()` on wakeup. See
 * docs/hal/sleep.md for the wakeup-source matrix and per-target notes.
 */

#ifndef BLUSYS_SLEEP_H
#define BLUSYS_SLEEP_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Wakeup cause reported by ::blusys_sleep_get_wakeup_cause. */
typedef enum {
    BLUSYS_SLEEP_WAKEUP_UNDEFINED = 0,  /**< Cold boot or unknown reset reason. */
    BLUSYS_SLEEP_WAKEUP_TIMER,          /**< RTC timer expired. */
    BLUSYS_SLEEP_WAKEUP_EXT0,           /**< EXT0 single-pin RTC wakeup (ESP32 / ESP32-S3 only). */
    BLUSYS_SLEEP_WAKEUP_EXT1,           /**< EXT1 pin-mask RTC wakeup (deep sleep only). */
    BLUSYS_SLEEP_WAKEUP_TOUCHPAD,       /**< Touch-pad wakeup (ESP32 / ESP32-S3 only). */
    BLUSYS_SLEEP_WAKEUP_GPIO,           /**< Light-sleep GPIO wakeup. */
    BLUSYS_SLEEP_WAKEUP_UART,           /**< UART RX wakeup (light sleep only). */
    BLUSYS_SLEEP_WAKEUP_ULP,            /**< ULP coprocessor wakeup. */
} blusys_sleep_wakeup_t;

/**
 * @brief Arm the RTC timer as a wakeup source.
 * @param us  Sleep duration in microseconds; must be greater than zero.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero duration, or a
 *         translated ESP-IDF error on failure.
 */
blusys_err_t blusys_sleep_enable_timer_wakeup(uint64_t us);

/**
 * @brief Enable the ULP coprocessor as a deep-sleep wakeup source.
 *
 * Use together with `blusys_ulp_start()`. Returns a translated ESP-IDF error
 * if ULP is unavailable or not enabled in sdkconfig.
 */
blusys_err_t blusys_sleep_enable_ulp_wakeup(void);

/**
 * @brief Enter light sleep.
 *
 * CPU halts and RAM/peripheral state is preserved. Returns after the first
 * armed wakeup source fires; execution continues from the next instruction.
 */
blusys_err_t blusys_sleep_enter_light(void);

/**
 * @brief Enter deep sleep. Does not return.
 *
 * The device resets on wakeup and execution restarts from `app_main()`. Only
 * RTC slow memory and RTC peripherals remain active; store any state that
 * must survive in NVS or `RTC_DATA_ATTR` before calling.
 */
blusys_err_t blusys_sleep_enter_deep(void);

/**
 * @brief Wakeup cause for the most recent sleep exit.
 *
 * Call at the start of `app_main()` (after deep sleep) or after
 * ::blusys_sleep_enter_light returns. Returns
 * ::BLUSYS_SLEEP_WAKEUP_UNDEFINED after a cold power-on or unknown reset.
 */
blusys_sleep_wakeup_t blusys_sleep_get_wakeup_cause(void);

#ifdef __cplusplus
}
#endif

#endif
