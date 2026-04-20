/**
 * @file system.h
 * @brief Runtime helpers common across all supported targets.
 *
 * Soft restart, uptime, reset reason, and heap statistics. This module is
 * stateless — no handle. See docs/hal/system.md.
 */

#ifndef BLUSYS_SYSTEM_H
#define BLUSYS_SYSTEM_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Reason for the most recent reset. */
typedef enum {
    BLUSYS_RESET_REASON_UNKNOWN = 0,           /**< Unmapped or unrecognized reset reason. */
    BLUSYS_RESET_REASON_POWER_ON,              /**< Cold power-on. */
    BLUSYS_RESET_REASON_EXTERNAL,              /**< External reset pin. */
    BLUSYS_RESET_REASON_SOFTWARE,              /**< Software-triggered restart (e.g. ::blusys_system_restart). */
    BLUSYS_RESET_REASON_PANIC,                 /**< Exception, panic handler, or abort. */
    BLUSYS_RESET_REASON_INTERRUPT_WATCHDOG,    /**< Interrupt watchdog timeout. */
    BLUSYS_RESET_REASON_TASK_WATCHDOG,         /**< Task watchdog timeout. */
    BLUSYS_RESET_REASON_WATCHDOG,              /**< Other watchdog timeout (RTC or MWDT). */
    BLUSYS_RESET_REASON_DEEP_SLEEP,            /**< Wakeup from deep sleep. */
    BLUSYS_RESET_REASON_BROWNOUT,              /**< Supply voltage dropped below the brownout threshold. */
    BLUSYS_RESET_REASON_SDIO,                  /**< SDIO peripheral reset. */
    BLUSYS_RESET_REASON_USB,                   /**< USB peripheral reset. */
    BLUSYS_RESET_REASON_JTAG,                  /**< JTAG-initiated reset. */
    BLUSYS_RESET_REASON_EFUSE,                 /**< eFuse-related reset. */
    BLUSYS_RESET_REASON_POWER_GLITCH,          /**< Power glitch detector fired. */
    BLUSYS_RESET_REASON_CPU_LOCKUP,            /**< CPU lockup detection. */
} blusys_reset_reason_t;

/**
 * @brief Trigger a software reset. Does not return.
 *
 * The next boot reports ::BLUSYS_RESET_REASON_SOFTWARE.
 */
void blusys_system_restart(void);

/**
 * @brief Microseconds elapsed since boot.
 * @param out_us  Output pointer for the uptime value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_us is NULL.
 * @note Uptime resets on every reset or deep-sleep wakeup.
 */
blusys_err_t blusys_system_uptime_us(uint64_t *out_us);

/**
 * @brief Reason for the most recent reset.
 * @param out_reason  Output pointer for the reset reason enum.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_reason is NULL.
 */
blusys_err_t blusys_system_reset_reason(blusys_reset_reason_t *out_reason);

/**
 * @brief Current free heap size in bytes.
 * @param out_bytes  Output pointer for the heap size.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_bytes is NULL.
 */
blusys_err_t blusys_system_free_heap_bytes(size_t *out_bytes);

/**
 * @brief Minimum free heap size recorded since boot.
 *
 * Useful as a low-water mark for detecting memory pressure over time.
 *
 * @param out_bytes  Output pointer for the watermark value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_bytes is NULL.
 */
blusys_err_t blusys_system_minimum_free_heap_bytes(size_t *out_bytes);

#ifdef __cplusplus
}
#endif

#endif
