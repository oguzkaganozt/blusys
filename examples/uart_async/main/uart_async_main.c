#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_UART_ASYNC_PORT CONFIG_BLUSYS_UART_ASYNC_PORT
#define BLUSYS_UART_ASYNC_TX_PIN CONFIG_BLUSYS_UART_ASYNC_TX_PIN
#define BLUSYS_UART_ASYNC_RX_PIN CONFIG_BLUSYS_UART_ASYNC_RX_PIN
#define BLUSYS_UART_ASYNC_BAUDRATE 115200u
#define BLUSYS_UART_ASYNC_TX_DONE_BIT (1u << 0)
#define BLUSYS_UART_ASYNC_RX_DONE_BIT (1u << 1)

typedef struct {
    TaskHandle_t task;
    char rx_buffer[64];
    size_t rx_size;
    blusys_err_t tx_status;
} blusys_uart_async_ctx_t;

static void blusys_uart_async_on_tx(blusys_uart_t *uart, blusys_err_t status, void *user_ctx)
{
    blusys_uart_async_ctx_t *ctx = user_ctx;

    (void) uart;

    ctx->tx_status = status;
    xTaskNotify(ctx->task, BLUSYS_UART_ASYNC_TX_DONE_BIT, eSetBits);
}

static void blusys_uart_async_on_rx(blusys_uart_t *uart, const void *data, size_t size, void *user_ctx)
{
    blusys_uart_async_ctx_t *ctx = user_ctx;
    size_t copy_size = (size > (sizeof(ctx->rx_buffer) - 1u)) ? (sizeof(ctx->rx_buffer) - 1u) : size;

    (void) uart;

    memcpy(ctx->rx_buffer, data, copy_size);
    ctx->rx_buffer[copy_size] = '\0';
    ctx->rx_size = copy_size;
    xTaskNotify(ctx->task, BLUSYS_UART_ASYNC_RX_DONE_BIT, eSetBits);
}

void app_main(void)
{
    static const char tx_message[] = "blusys-uart-async";
    blusys_uart_async_ctx_t ctx = {
        .task = NULL,
        .rx_buffer = {0},
        .rx_size = 0u,
        .tx_status = BLUSYS_OK,
    };
    blusys_uart_t *uart = NULL;
    blusys_err_t err;
    uint32_t notified_bits = 0u;

    ctx.task = xTaskGetCurrentTaskHandle();

    printf("Blusys uart async\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_UART_ASYNC_PORT);
    printf("tx_pin: %d\n", BLUSYS_UART_ASYNC_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_UART_ASYNC_RX_PIN);
    printf("wire tx_pin to rx_pin before running this example\n");

    err = blusys_uart_open(BLUSYS_UART_ASYNC_PORT,
                           BLUSYS_UART_ASYNC_TX_PIN,
                           BLUSYS_UART_ASYNC_RX_PIN,
                           BLUSYS_UART_ASYNC_BAUDRATE,
                           &uart);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_uart_set_tx_callback(uart, blusys_uart_async_on_tx, &ctx);
    if (err != BLUSYS_OK) {
        printf("set tx callback error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    err = blusys_uart_set_rx_callback(uart, blusys_uart_async_on_rx, &ctx);
    if (err != BLUSYS_OK) {
        printf("set rx callback error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    err = blusys_uart_write_async(uart, tx_message, sizeof(tx_message) - 1u);
    if (err != BLUSYS_OK) {
        printf("write async error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    while ((notified_bits & (BLUSYS_UART_ASYNC_TX_DONE_BIT | BLUSYS_UART_ASYNC_RX_DONE_BIT)) !=
           (BLUSYS_UART_ASYNC_TX_DONE_BIT | BLUSYS_UART_ASYNC_RX_DONE_BIT)) {
        xTaskNotifyWait(0u, UINT32_MAX, &notified_bits, portMAX_DELAY);
    }

    printf("tx_status: %s\n", blusys_err_string(ctx.tx_status));
    printf("tx: %s\n", tx_message);
    printf("rx_bytes: %u\n", (unsigned int) ctx.rx_size);
    printf("rx: %.*s\n", (int) ctx.rx_size, ctx.rx_buffer);
    printf("async_result: %s\n",
           ((ctx.tx_status == BLUSYS_OK) &&
            (ctx.rx_size == (sizeof(tx_message) - 1u)) &&
            (memcmp(ctx.rx_buffer, tx_message, sizeof(tx_message) - 1u) == 0)) ?
               "ok" :
               "incomplete");

    err = blusys_uart_close(uart);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
