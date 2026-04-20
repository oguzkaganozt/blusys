/* src/framework/test/fake_hal/uart.c — in-memory UART for tests.
 *
 * One slot per port number. TX writes are captured in a drain-able byte
 * buffer; RX pushes are handed to the registered rx_callback synchronously.
 * The implementation is small by design — tests need enough to compile and
 * exercise a code path, not cycle-accurate UART emulation.
 */

#include "blusys/framework/test/fake_hal.h"
#include "blusys/hal/uart.h"

#include <stdlib.h>
#include <string.h>

#define FAKE_UART_MAX_PORTS   4
#define FAKE_UART_TX_BUF      512

struct blusys_uart {
    int                         port;
    bool                        alive;
    blusys_uart_rx_callback_t   rx_cb;
    void                       *rx_ctx;
    blusys_uart_tx_callback_t   tx_cb;
    void                       *tx_ctx;
    uint8_t                     tx_buf[FAKE_UART_TX_BUF];
    size_t                      tx_len;
};

static struct blusys_uart g_uarts[FAKE_UART_MAX_PORTS];

static struct blusys_uart *port_slot(int port)
{
    if (port < 0 || port >= FAKE_UART_MAX_PORTS) return NULL;
    return &g_uarts[port];
}

blusys_err_t blusys_uart_open(int port, int tx_pin, int rx_pin,
                              uint32_t baudrate, blusys_uart_t **out_uart)
{
    (void)tx_pin; (void)rx_pin; (void)baudrate;
    if (out_uart == NULL) return BLUSYS_ERR_INVALID_ARG;
    struct blusys_uart *u = port_slot(port);
    if (u == NULL) return BLUSYS_ERR_INVALID_ARG;
    if (u->alive) return BLUSYS_ERR_INVALID_STATE;
    memset(u, 0, sizeof(*u));
    u->port   = port;
    u->alive  = true;
    *out_uart = u;
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_close(blusys_uart_t *uart)
{
    if (uart == NULL) return BLUSYS_ERR_INVALID_ARG;
    uart->alive = false;
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_write(blusys_uart_t *uart, const void *data,
                               size_t size, int timeout_ms)
{
    (void)timeout_ms;
    if (uart == NULL || (data == NULL && size > 0)) return BLUSYS_ERR_INVALID_ARG;
    size_t room = FAKE_UART_TX_BUF - uart->tx_len;
    size_t n    = (size < room) ? size : room;
    memcpy(uart->tx_buf + uart->tx_len, data, n);
    uart->tx_len += n;
    if (uart->tx_cb) uart->tx_cb(uart, BLUSYS_OK, uart->tx_ctx);
    return (n == size) ? BLUSYS_OK : BLUSYS_ERR_NO_MEM;
}

blusys_err_t blusys_uart_read(blusys_uart_t *uart, void *data,
                              size_t size, int timeout_ms, size_t *out_read)
{
    (void)uart; (void)data; (void)size; (void)timeout_ms;
    /* Synchronous read isn't supported by the fake — callers should use the
     * rx_callback path driven by blusys_test_uart_push_rx(). */
    if (out_read) *out_read = 0;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_uart_flush_rx(blusys_uart_t *uart)
{
    (void)uart;
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_set_tx_callback(blusys_uart_t *uart,
                                         blusys_uart_tx_callback_t cb, void *user_ctx)
{
    if (uart == NULL) return BLUSYS_ERR_INVALID_ARG;
    uart->tx_cb  = cb;
    uart->tx_ctx = user_ctx;
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_set_rx_callback(blusys_uart_t *uart,
                                         blusys_uart_rx_callback_t cb, void *user_ctx)
{
    if (uart == NULL) return BLUSYS_ERR_INVALID_ARG;
    uart->rx_cb  = cb;
    uart->rx_ctx = user_ctx;
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_write_async(blusys_uart_t *uart, const void *data, size_t size)
{
    return blusys_uart_write(uart, data, size, 0);
}

/* ── test inject API ──────────────────────────────────────────────────── */

void blusys_test_uart_push_rx(int port, const void *data, size_t size)
{
    struct blusys_uart *u = port_slot(port);
    if (u == NULL || !u->alive || u->rx_cb == NULL) return;
    u->rx_cb(u, data, size, u->rx_ctx);
}

size_t blusys_test_uart_take_tx(int port, void *out, size_t out_size)
{
    struct blusys_uart *u = port_slot(port);
    if (u == NULL || !u->alive || out == NULL) return 0;
    size_t n = (u->tx_len < out_size) ? u->tx_len : out_size;
    memcpy(out, u->tx_buf, n);
    u->tx_len = 0;
    return n;
}

void blusys_test_uart_reset_all(void)
{
    memset(g_uarts, 0, sizeof(g_uarts));
}
