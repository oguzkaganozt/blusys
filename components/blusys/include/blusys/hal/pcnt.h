/**
 * @file pcnt.h
 * @brief Pulse counter input with edge selection, watch-point callbacks, and glitch filtering.
 *
 * Not supported on ESP32-C3 (every call returns `BLUSYS_ERR_NOT_SUPPORTED`).
 * See docs/hal/pcnt.md.
 */

#ifndef BLUSYS_PCNT_H
#define BLUSYS_PCNT_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a pulse counter unit. */
typedef struct blusys_pcnt blusys_pcnt_t;

/** @brief Which signal transitions count as pulses. */
typedef enum {
    BLUSYS_PCNT_EDGE_RISING = 0,  /**< Count rising edges only. */
    BLUSYS_PCNT_EDGE_FALLING,     /**< Count falling edges only. */
    BLUSYS_PCNT_EDGE_BOTH,        /**< Count both edges. */
} blusys_pcnt_edge_t;

/**
 * @brief Watch-point callback: fires in ISR context when the counter hits a threshold.
 * @param pcnt         Pulse-counter handle.
 * @param watch_point  Threshold value that triggered the event.
 * @param user_ctx     User context pointer provided at registration.
 * @return `true` to request a FreeRTOS yield.
 * @warning Runs in ISR context. Must not block or allocate.
 */
typedef bool (*blusys_pcnt_callback_t)(blusys_pcnt_t *pcnt, int watch_point, void *user_ctx);

/**
 * @brief Open a pulse counter on the given pin.
 *
 * Counting begins only after ::blusys_pcnt_start.
 *
 * @param pin             GPIO number.
 * @param edge            Edges to count.
 * @param max_glitch_ns   Glitch filter threshold in nanoseconds; pulses
 *                        shorter than this are ignored (0 = disabled).
 * @param out_pcnt        Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid arguments,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on ESP32-C3.
 */
blusys_err_t blusys_pcnt_open(int pin,
                              blusys_pcnt_edge_t edge,
                              uint32_t max_glitch_ns,
                              blusys_pcnt_t **out_pcnt);

/** @brief Stop counting and release the handle. */
blusys_err_t blusys_pcnt_close(blusys_pcnt_t *pcnt);

/**
 * @brief Enable the counter.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already running.
 */
blusys_err_t blusys_pcnt_start(blusys_pcnt_t *pcnt);

/**
 * @brief Disable the counter without clearing the count.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already stopped.
 */
blusys_err_t blusys_pcnt_stop(blusys_pcnt_t *pcnt);

/** @brief Reset the counter to zero. */
blusys_err_t blusys_pcnt_clear_count(blusys_pcnt_t *pcnt);

/**
 * @brief Read the current counter value.
 * @param pcnt       Handle.
 * @param out_count  Destination for the count.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_count is NULL.
 */
blusys_err_t blusys_pcnt_get_count(blusys_pcnt_t *pcnt, int *out_count);

/**
 * @brief Add a watch-point threshold. The callback fires when the counter hits this value.
 *
 * Must be called while the counter is stopped.
 *
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the counter is running.
 */
blusys_err_t blusys_pcnt_add_watch_point(blusys_pcnt_t *pcnt, int count);

/**
 * @brief Remove a previously added watch-point.
 *
 * Must be called while the counter is stopped.
 */
blusys_err_t blusys_pcnt_remove_watch_point(blusys_pcnt_t *pcnt, int count);

/**
 * @brief Register the ISR callback for watch-point events.
 *
 * Must be called while the counter is stopped.
 *
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if the counter is running.
 */
blusys_err_t blusys_pcnt_set_callback(blusys_pcnt_t *pcnt,
                                      blusys_pcnt_callback_t callback,
                                      void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
