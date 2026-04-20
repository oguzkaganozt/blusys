/**
 * @file wdt.h
 * @brief Task watchdog timer: panic or log when a monitored task fails to feed in time.
 *
 * Global (not handle-based). One controlling task calls init/deinit; each
 * monitored task calls subscribe/feed/unsubscribe. See docs/hal/wdt.md.
 */

#ifndef BLUSYS_WDT_H
#define BLUSYS_WDT_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure and enable the task watchdog.
 *
 * Call once from the controlling task.
 *
 * @param timeout_ms        Watchdog timeout in milliseconds; must be > 0.
 * @param panic_on_timeout  `true` causes a system panic (core dump or reboot)
 *                          on expiry; `false` logs only.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for zero timeout,
 *         translated ESP-IDF error on double-init.
 */
blusys_err_t blusys_wdt_init(uint32_t timeout_ms, bool panic_on_timeout);

/**
 * @brief Disable the task watchdog.
 *
 * All tasks should unsubscribe before calling this.
 */
blusys_err_t blusys_wdt_deinit(void);

/**
 * @brief Subscribe the calling task to the watchdog.
 * @return `BLUSYS_OK`, translated ESP-IDF error if already subscribed.
 */
blusys_err_t blusys_wdt_subscribe(void);

/**
 * @brief Unsubscribe the calling task from the watchdog.
 *
 * Call before the task exits.
 */
blusys_err_t blusys_wdt_unsubscribe(void);

/**
 * @brief Reset the watchdog timeout for the calling task.
 *
 * Must be called within `timeout_ms` of the previous feed (or the subscribe call).
 *
 * @return `BLUSYS_OK`, translated ESP-IDF error if the calling task is not subscribed.
 * @warning Must not be called from an ISR context.
 */
blusys_err_t blusys_wdt_feed(void);

#ifdef __cplusplus
}
#endif

#endif
