#ifndef BLUSYS_UART_H
#define BLUSYS_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_uart blusys_uart_t;

typedef void (*blusys_uart_tx_callback_t)(blusys_uart_t *uart, blusys_err_t status, void *user_ctx);
typedef void (*blusys_uart_rx_callback_t)(blusys_uart_t *uart,
                                          const void *data,
                                          size_t size,
                                          void *user_ctx);

blusys_err_t blusys_uart_open(int port,
                              int tx_pin,
                              int rx_pin,
                              uint32_t baudrate,
                              blusys_uart_t **out_uart);
blusys_err_t blusys_uart_close(blusys_uart_t *uart);
blusys_err_t blusys_uart_write(blusys_uart_t *uart, const void *data, size_t size, int timeout_ms);
blusys_err_t blusys_uart_read(blusys_uart_t *uart,
                              void *data,
                              size_t size,
                              int timeout_ms,
                              size_t *out_read);
blusys_err_t blusys_uart_flush_rx(blusys_uart_t *uart);
blusys_err_t blusys_uart_set_tx_callback(blusys_uart_t *uart,
                                         blusys_uart_tx_callback_t callback,
                                         void *user_ctx);
blusys_err_t blusys_uart_set_rx_callback(blusys_uart_t *uart,
                                         blusys_uart_rx_callback_t callback,
                                         void *user_ctx);
blusys_err_t blusys_uart_write_async(blusys_uart_t *uart, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
