/**
 * @file twai.h
 * @brief Classic CAN-style communication using the ESP32 TWAI controller.
 *
 * Standard 11-bit frames with an async RX callback that runs in ISR context.
 * External CAN transceiver hardware is required. See docs/hal/twai.md.
 */

#ifndef BLUSYS_TWAI_H
#define BLUSYS_TWAI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for the TWAI controller. */
typedef struct blusys_twai blusys_twai_t;

/** @brief One classic CAN frame (11-bit ID). */
typedef struct {
    uint32_t id;            /**< 11-bit standard CAN identifier. */
    bool     remote_frame;  /**< `true` for remote frames (RTR bit). */
    uint8_t  dlc;           /**< Data length code (0–8). */
    uint8_t  data[8];       /**< Payload bytes. */
    size_t   data_len;      /**< Number of valid bytes in @p data; 0 for remote frames. */
} blusys_twai_frame_t;

/**
 * @brief Callback invoked in ISR context when a frame is received.
 * @param twai      TWAI handle.
 * @param frame     Received frame (valid only during this callback).
 * @param user_ctx  User context pointer provided at registration.
 * @return `true` to request a FreeRTOS yield.
 * @warning Runs in ISR context. Must not block or allocate.
 */
typedef bool (*blusys_twai_rx_callback_t)(blusys_twai_t *twai,
                                          const blusys_twai_frame_t *frame,
                                          void *user_ctx);

/**
 * @brief Install the TWAI driver. Does not join the bus — call ::blusys_twai_start.
 *
 * @param tx_pin    GPIO for TWAI TX (connects to transceiver TXD).
 * @param rx_pin    GPIO for TWAI RX (connects to transceiver RXD).
 * @param bitrate   Bus bitrate in bits/s (e.g. 125000, 500000, 1000000).
 * @param out_twai  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.
 */
blusys_err_t blusys_twai_open(int tx_pin,
                              int rx_pin,
                              uint32_t bitrate,
                              blusys_twai_t **out_twai);

/**
 * @brief Uninstall the TWAI driver and free its handle.
 *
 * Call ::blusys_twai_stop first.
 */
blusys_err_t blusys_twai_close(blusys_twai_t *twai);

/** @brief Join the CAN bus. Received frames will trigger the RX callback; writes are allowed. */
blusys_err_t blusys_twai_start(blusys_twai_t *twai);

/** @brief Leave the bus. No frames are transmitted or received after this call. */
blusys_err_t blusys_twai_stop(blusys_twai_t *twai);

/**
 * @brief Transmit one frame. Blocks until sent or timeout.
 *
 * @param twai        Handle.
 * @param frame       Frame to send; `id` must be a valid 11-bit value, `dlc` must match `data_len`.
 * @param timeout_ms  Milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` for indefinite.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_STATE` if called before ::blusys_twai_start,
 *         `BLUSYS_ERR_TIMEOUT`,
 *         `BLUSYS_ERR_IO` if the backend reports transmit failure,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid frame fields.
 */
blusys_err_t blusys_twai_write(blusys_twai_t *twai,
                               const blusys_twai_frame_t *frame,
                               int timeout_ms);

/**
 * @brief Register the ISR callback for received frames. Pass NULL to clear.
 * @param twai      Handle.
 * @param callback  ISR-context callback, or NULL to clear.
 * @param user_ctx  Opaque pointer passed back to the callback unchanged.
 */
blusys_err_t blusys_twai_set_rx_callback(blusys_twai_t *twai,
                                         blusys_twai_rx_callback_t callback,
                                         void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
