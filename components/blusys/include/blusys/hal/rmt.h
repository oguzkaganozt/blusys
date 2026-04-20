/**
 * @file rmt.h
 * @brief Timed digital pulse sequences: TX (generate IR codes etc.), RX (capture pulses).
 *
 * Internal resolution is 1 MHz (1 µs per tick). See docs/hal/rmt.md.
 */

#ifndef BLUSYS_RMT_H
#define BLUSYS_RMT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief One pulse in an RMT waveform. Shared by TX and RX. */
typedef struct {
    bool     level;        /**< Signal level: `true` = high, `false` = low. */
    uint32_t duration_us;  /**< Pulse duration in microseconds; must be > 0. */
} blusys_rmt_pulse_t;

/* ── TX ───────────────────────────────────────────────────────────────── */

/** @brief Opaque TX handle. */
typedef struct blusys_rmt blusys_rmt_t;

/**
 * @brief Open an RMT TX channel on the given pin.
 * @param pin         GPIO number.
 * @param idle_level  Signal level when idle between transmissions.
 * @param out_rmt     Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.
 */
blusys_err_t blusys_rmt_open(int pin, bool idle_level, blusys_rmt_t **out_rmt);

/** @brief Release the RMT TX channel and free its handle. */
blusys_err_t blusys_rmt_close(blusys_rmt_t *rmt);

/**
 * @brief Send a pulse sequence. Blocks until complete or timeout.
 * @param rmt          TX handle.
 * @param pulses       Array of pulses to send.
 * @param pulse_count  Number of pulses; must be > 0.
 * @param timeout_ms   Milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` for indefinite.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for empty list or zero durations,
 *         `BLUSYS_ERR_TIMEOUT` on transmission timeout.
 */
blusys_err_t blusys_rmt_write(blusys_rmt_t *rmt,
                              const blusys_rmt_pulse_t *pulses,
                              size_t pulse_count,
                              int timeout_ms);

/* ── RX ───────────────────────────────────────────────────────────────── */

/** @brief Opaque RX handle. */
typedef struct blusys_rmt_rx blusys_rmt_rx_t;

/** @brief RX glitch-filter and idle-detection thresholds. */
typedef struct {
    uint32_t signal_range_min_ns;  /**< Glitch filter: pulses shorter than this are ignored. */
    uint32_t signal_range_max_ns;  /**< Idle threshold: silence longer than this ends the frame. */
} blusys_rmt_rx_config_t;

/**
 * @brief Open an RMT RX channel on the given pin.
 * @param pin          GPIO number.
 * @param config       Glitch filter + idle threshold settings.
 * @param out_rmt_rx   Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.
 */
blusys_err_t blusys_rmt_rx_open(int pin,
                                 const blusys_rmt_rx_config_t *config,
                                 blusys_rmt_rx_t **out_rmt_rx);

/** @brief Release the RMT RX channel and free its handle. */
blusys_err_t blusys_rmt_rx_close(blusys_rmt_rx_t *rmt_rx);

/**
 * @brief Wait for a pulse frame to complete and fill @p pulses with the captured sequence.
 *
 * A frame ends when the RX line has been idle for longer than the
 * `signal_range_max_ns` threshold set at open time.
 *
 * @param rmt_rx      RX handle.
 * @param pulses      Buffer for captured pulses.
 * @param max_pulses  Buffer capacity.
 * @param out_count   Number of pulses captured.
 * @param timeout_ms  Milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` for indefinite.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if no frame completes before the timeout.
 */
blusys_err_t blusys_rmt_rx_read(blusys_rmt_rx_t *rmt_rx,
                                 blusys_rmt_pulse_t *pulses,
                                 size_t max_pulses,
                                 size_t *out_count,
                                 int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
