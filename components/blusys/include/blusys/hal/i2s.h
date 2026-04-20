/**
 * @file i2s.h
 * @brief Stereo audio streaming over I2S: TX for playback, RX for capture.
 *
 * Fixed format: Philips standard mode, master mode, stereo, 16-bit samples.
 * TX and RX are independent handles; full-duplex on one port is not supported.
 * See docs/hal/i2s.md.
 */

#ifndef BLUSYS_I2S_H
#define BLUSYS_I2S_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── TX ───────────────────────────────────────────────────────────────── */

/** @brief Opaque handle for an I2S TX channel. */
typedef struct blusys_i2s_tx blusys_i2s_tx_t;

/** @brief Pin and clock configuration for an I2S TX channel. */
typedef struct {
    int      port;            /**< I2S controller index. */
    int      bclk_pin;        /**< Bit-clock GPIO. */
    int      ws_pin;          /**< Word-select (LRCLK) GPIO. */
    int      dout_pin;        /**< Data-out GPIO. */
    int      mclk_pin;        /**< Master-clock GPIO; set to `-1` if unused. */
    uint32_t sample_rate_hz;  /**< Audio sample rate (e.g. 16000, 44100). */
} blusys_i2s_tx_config_t;

/**
 * @brief Open an I2S TX channel. Output is not active until ::blusys_i2s_tx_start.
 * @param config    Pin + clock configuration.
 * @param out_i2s   Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` on invalid config
 *         (NULL, invalid pins, zero sample rate).
 */
blusys_err_t blusys_i2s_tx_open(const blusys_i2s_tx_config_t *config,
                                blusys_i2s_tx_t **out_i2s);

/** @brief Stop and release an I2S TX channel. */
blusys_err_t blusys_i2s_tx_close(blusys_i2s_tx_t *i2s);

/** @brief Enable the I2S clock and start DMA transfer. */
blusys_err_t blusys_i2s_tx_start(blusys_i2s_tx_t *i2s);

/** @brief Stop DMA transfer and disable the I2S clock. */
blusys_err_t blusys_i2s_tx_stop(blusys_i2s_tx_t *i2s);

/**
 * @brief Write interleaved 16-bit stereo samples.
 *
 * Provide L and R samples alternating: `[L0, R0, L1, R1, ...]`.
 *
 * @param i2s         TX handle.
 * @param data        Sample buffer.
 * @param size        Buffer size in bytes.
 * @param timeout_ms  Milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_STATE` if called before ::blusys_i2s_tx_start,
 *         `BLUSYS_ERR_TIMEOUT`.
 */
blusys_err_t blusys_i2s_tx_write(blusys_i2s_tx_t *i2s,
                                 const void *data,
                                 size_t size,
                                 int timeout_ms);

/* ── RX ───────────────────────────────────────────────────────────────── */

/** @brief Opaque handle for an I2S RX channel. */
typedef struct blusys_i2s_rx blusys_i2s_rx_t;

/** @brief Pin and clock configuration for an I2S RX channel. */
typedef struct {
    int      port;            /**< I2S controller index. */
    int      bclk_pin;        /**< Bit-clock GPIO. */
    int      ws_pin;          /**< Word-select (LRCLK) GPIO. */
    int      din_pin;         /**< Data-in GPIO. */
    int      mclk_pin;        /**< Master-clock GPIO; set to `-1` if unused. */
    uint32_t sample_rate_hz;  /**< Audio sample rate. */
} blusys_i2s_rx_config_t;

/**
 * @brief Open an I2S RX channel. Capture does not begin until ::blusys_i2s_rx_start.
 * @param config    Pin + clock configuration.
 * @param out_i2s   Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid config.
 */
blusys_err_t blusys_i2s_rx_open(const blusys_i2s_rx_config_t *config,
                                blusys_i2s_rx_t **out_i2s);

/** @brief Stop and release an I2S RX channel. */
blusys_err_t blusys_i2s_rx_close(blusys_i2s_rx_t *i2s);

/** @brief Enable the I2S clock and begin capture. */
blusys_err_t blusys_i2s_rx_start(blusys_i2s_rx_t *i2s);

/** @brief Stop capture and disable the I2S clock. */
blusys_err_t blusys_i2s_rx_stop(blusys_i2s_rx_t *i2s);

/**
 * @brief Read captured audio samples (interleaved 16-bit stereo) into @p buf.
 *
 * @param i2s         RX handle.
 * @param buf         Destination buffer.
 * @param size        Buffer size in bytes.
 * @param bytes_read  Actual bytes received.
 * @param timeout_ms  Milliseconds to wait.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`.
 */
blusys_err_t blusys_i2s_rx_read(blusys_i2s_rx_t *i2s,
                                void *buf,
                                size_t size,
                                size_t *bytes_read,
                                int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
