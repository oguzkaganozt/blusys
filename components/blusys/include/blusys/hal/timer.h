/**
 * @file timer.h
 * @brief General-purpose timer with microsecond resolution (periodic or one-shot).
 *
 * Alarms fire in ISR context via a registered callback. See docs/hal/timer.md
 * for ISR rules, thread-safety notes, and the full lifecycle.
 */

#ifndef BLUSYS_TIMER_H
#define BLUSYS_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a general-purpose timer. */
typedef struct blusys_timer blusys_timer_t;

/**
 * @brief Callback invoked in GPTimer ISR context when the alarm fires.
 * @param timer     Timer that triggered the callback.
 * @param user_ctx  User context pointer provided at registration.
 * @return `true` to request a FreeRTOS yield after waking a higher-priority task.
 * @warning Runs in ISR context. Must not block or call blocking Blusys APIs.
 */
typedef bool (*blusys_timer_callback_t)(blusys_timer_t *timer, void *user_ctx);

/**
 * @brief Allocate a GPTimer with the given period.
 *
 * The timer is not started until ::blusys_timer_start.
 *
 * @param period_us  Alarm period in microseconds; must be > 0.
 * @param periodic   `true` for repeating, `false` for one-shot.
 * @param out_timer  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero period or NULL pointer.
 */
blusys_err_t blusys_timer_open(uint32_t period_us, bool periodic, blusys_timer_t **out_timer);

/**
 * @brief Stop the timer if running and release its handle.
 * @param timer Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p timer is NULL.
 */
blusys_err_t blusys_timer_close(blusys_timer_t *timer);

/**
 * @brief Arm the timer. The callback fires after each period expires.
 * @param timer Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already running.
 */
blusys_err_t blusys_timer_start(blusys_timer_t *timer);

/**
 * @brief Disarm the timer. Reconfiguration and restart are allowed after this.
 * @param timer Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already stopped.
 */
blusys_err_t blusys_timer_stop(blusys_timer_t *timer);

/**
 * @brief Change the alarm period. Must be called while the timer is stopped.
 * @param timer      Handle.
 * @param period_us  New alarm period in microseconds; must be > 0.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the timer is running.
 */
blusys_err_t blusys_timer_set_period(blusys_timer_t *timer, uint32_t period_us);

/**
 * @brief Register or replace the ISR callback.
 *
 * Must be called while the timer is stopped.
 *
 * @param timer     Handle.
 * @param callback  Callback invoked in ISR context when the alarm fires.
 * @param user_ctx  Opaque pointer passed back to the callback unchanged.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the timer is running.
 */
blusys_err_t blusys_timer_set_callback(blusys_timer_t *timer,
                                       blusys_timer_callback_t callback,
                                       void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
