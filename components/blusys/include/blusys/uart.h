#ifndef BLUSYS_UART_H
#define BLUSYS_UART_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_uart blusys_uart_t;

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

#ifdef __cplusplus
}
#endif

#endif
