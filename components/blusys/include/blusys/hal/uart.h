/**
 * @file uart.h
 * @brief Blocking serial IO with an optional async callback layer. 8N1 line format.
 *
 * See docs/hal/uart.md for thread-safety interactions between blocking and
 * async modes, and for ISR notes.
 */

#ifndef BLUSYS_UART_H
#define BLUSYS_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a configured UART port. */
typedef struct blusys_uart blusys_uart_t;

/**
 * @brief Task-context callback: fires when an async TX operation completes or fails.
 * @param uart      UART whose async TX completed.
 * @param status    `BLUSYS_OK` on success or an error code.
 * @param user_ctx  User context pointer provided at registration.
 */
typedef void (*blusys_uart_tx_callback_t)(blusys_uart_t *uart, blusys_err_t status, void *user_ctx);

/**
 * @brief Task-context callback: fires when received bytes are available.
 *
 * The @p data pointer is valid only for the duration of the callback.
 *
 * @param uart      UART that received the bytes.
 * @param data      Received bytes (valid only during this callback).
 * @param size      Number of bytes pointed to by @p data.
 * @param user_ctx  User context pointer provided at registration.
 */
typedef void (*blusys_uart_rx_callback_t)(blusys_uart_t *uart,
                                          const void *data,
                                          size_t size,
                                          void *user_ctx);

/**
 * @brief Open a UART port and install the driver. Line format is 8N1.
 *
 * @param port      UART port number (0, 1, or 2 depending on target).
 * @param tx_pin    GPIO for TX.
 * @param rx_pin    GPIO for RX.
 * @param baudrate  Bits per second.
 * @param out_uart  Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid arguments,
 *         `BLUSYS_ERR_INVALID_STATE` if the port already has a driver installed.
 */
blusys_err_t blusys_uart_open(int port,
                              int tx_pin,
                              int rx_pin,
                              uint32_t baudrate,
                              blusys_uart_t **out_uart);

/**
 * @brief Stop pending async operations, uninstall the driver, free the handle.
 * @param uart Handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` if @p uart is NULL,
 *         `BLUSYS_ERR_INVALID_STATE` if called from within a UART callback.
 * @warning Must not be called from within a UART async callback.
 */
blusys_err_t blusys_uart_close(blusys_uart_t *uart);

/**
 * @brief Write bytes to the UART, blocking until complete or timeout.
 *
 * @param uart        Handle.
 * @param data        Buffer to send.
 * @param size        Number of bytes.
 * @param timeout_ms  Milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` for indefinite.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if transmit doesn't complete in time.
 */
blusys_err_t blusys_uart_write(blusys_uart_t *uart, const void *data, size_t size, int timeout_ms);

/**
 * @brief Read up to @p size bytes, blocking until at least one arrives or timeout.
 *
 * Partial reads are reported through @p out_read.
 *
 * @param uart        Handle.
 * @param data        Receive buffer.
 * @param size        Maximum bytes to read.
 * @param timeout_ms  Milliseconds to wait.
 * @param out_read    Number of bytes actually received.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_TIMEOUT` if fewer than @p size bytes arrive in time
 *         (partial count still reported in @p out_read),
 *         `BLUSYS_ERR_INVALID_STATE` while an RX callback is registered.
 */
blusys_err_t blusys_uart_read(blusys_uart_t *uart,
                              void *data,
                              size_t size,
                              int timeout_ms,
                              size_t *out_read);

/**
 * @brief Discard any data waiting in the RX buffer.
 * @param uart Handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` if @p uart is NULL,
 *         `BLUSYS_ERR_INVALID_STATE` if an RX callback is registered or the handle is closing.
 */
blusys_err_t blusys_uart_flush_rx(blusys_uart_t *uart);

/**
 * @brief Register a task-context callback for async TX completion.
 *
 * Cannot be changed while an async TX is pending.
 *
 * @param uart      Handle.
 * @param callback  TX-complete callback (task context).
 * @param user_ctx  Opaque pointer passed back to the callback unchanged.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` while an async TX is in progress.
 */
blusys_err_t blusys_uart_set_tx_callback(blusys_uart_t *uart,
                                         blusys_uart_tx_callback_t callback,
                                         void *user_ctx);

/**
 * @brief Register a task-context callback for async RX data delivery.
 *
 * While an RX callback is active, ::blusys_uart_read must not be called.
 *
 * @param uart      Handle.
 * @param callback  RX-data callback (task context).
 * @param user_ctx  Opaque pointer passed back to the callback unchanged.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` if @p uart is NULL,
 *         `BLUSYS_ERR_INVALID_STATE` if the handle is closing.
 */
blusys_err_t blusys_uart_set_rx_callback(blusys_uart_t *uart,
                                         blusys_uart_rx_callback_t callback,
                                         void *user_ctx);

/**
 * @brief Queue one async TX operation and return immediately.
 *
 * The registered TX callback fires on completion.
 *
 * @param uart  Handle.
 * @param data  Buffer to send.
 * @param size  Number of bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if another async TX is already pending.
 */
blusys_err_t blusys_uart_write_async(blusys_uart_t *uart, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
