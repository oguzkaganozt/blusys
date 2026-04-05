#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

/*
 * Concurrency test: TX task and RX task operate on the same UART handle
 * simultaneously. With TX wired to RX (loopback), bytes written by the TX
 * task must appear at the RX task.
 *
 * TX task: sends ITERS fixed messages, waits a short delay between each.
 * RX task: reads until it has received all expected bytes, or until the
 *   deadline expires.
 *
 * Pass criteria: total bytes received equals total bytes sent, no unexpected
 * errors from either task.
 */

#define BLUSYS_CONCURRENCY_UART_PORT     CONFIG_BLUSYS_CONCURRENCY_UART_PORT
#define BLUSYS_CONCURRENCY_UART_TX_PIN   CONFIG_BLUSYS_CONCURRENCY_UART_TX_PIN
#define BLUSYS_CONCURRENCY_UART_RX_PIN   CONFIG_BLUSYS_CONCURRENCY_UART_RX_PIN
#define BLUSYS_CONCURRENCY_UART_BAUDRATE 115200u
#define BLUSYS_CONCURRENCY_UART_ITERS    50u
#define BLUSYS_CONCURRENCY_UART_TX_DELAY_MS 10

#define BLUSYS_CONCURRENCY_UART_TX_DONE (1u << 0)
#define BLUSYS_CONCURRENCY_UART_RX_DONE (1u << 1)
#define BLUSYS_CONCURRENCY_UART_DONE_MASK \
    (BLUSYS_CONCURRENCY_UART_TX_DONE | BLUSYS_CONCURRENCY_UART_RX_DONE)

static const char blusys_concurrency_uart_message[] = "blusys-concurrency-uart\n";

typedef struct {
    blusys_uart_t *uart;
    uint32_t bytes_transferred;
    uint32_t errors;
    TaskHandle_t report_task;
    uint32_t done_bit;
} blusys_concurrency_uart_ctx_t;

static void blusys_concurrency_uart_tx_worker(void *arg)
{
    blusys_concurrency_uart_ctx_t *ctx = arg;
    size_t msg_len = strlen(blusys_concurrency_uart_message);
    uint32_t i;
    blusys_err_t err;

    for (i = 0u; i < BLUSYS_CONCURRENCY_UART_ITERS; ++i) {
        err = blusys_uart_write(ctx->uart,
                                blusys_concurrency_uart_message,
                                msg_len,
                                1000);
        if (err != BLUSYS_OK) {
            printf("tx iter_%u write_error: %s\n", (unsigned int) i, blusys_err_string(err));
            ctx->errors += 1u;
        } else {
            ctx->bytes_transferred += (uint32_t) msg_len;
        }

        vTaskDelay(pdMS_TO_TICKS(BLUSYS_CONCURRENCY_UART_TX_DELAY_MS));
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

static void blusys_concurrency_uart_rx_worker(void *arg)
{
    blusys_concurrency_uart_ctx_t *ctx = arg;
    size_t msg_len = strlen(blusys_concurrency_uart_message);
    uint32_t total_expected = (uint32_t)(BLUSYS_CONCURRENCY_UART_ITERS * msg_len);
    uint8_t rx_buf[64];
    size_t bytes_read;
    blusys_err_t err;

    while (ctx->bytes_transferred < total_expected) {
        bytes_read = 0u;
        err = blusys_uart_read(ctx->uart, rx_buf, sizeof(rx_buf), 200, &bytes_read);

        if (err == BLUSYS_OK || err == BLUSYS_ERR_TIMEOUT) {
            ctx->bytes_transferred += (uint32_t) bytes_read;
        } else {
            printf("rx read_error: %s\n", blusys_err_string(err));
            ctx->errors += 1u;
            break;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

void app_main(void)
{
    blusys_concurrency_uart_ctx_t tx_ctx = {
        .uart = NULL,
        .bytes_transferred = 0u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = BLUSYS_CONCURRENCY_UART_TX_DONE,
    };
    blusys_concurrency_uart_ctx_t rx_ctx = {
        .uart = NULL,
        .bytes_transferred = 0u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = BLUSYS_CONCURRENCY_UART_RX_DONE,
    };
    blusys_uart_t *uart = NULL;
    blusys_err_t err;
    uint32_t notified_bits = 0u;
    uint32_t total_expected = (uint32_t)(BLUSYS_CONCURRENCY_UART_ITERS *
                                          strlen(blusys_concurrency_uart_message));

    printf("Blusys concurrency uart\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_CONCURRENCY_UART_PORT);
    printf("tx_pin: %d\n", BLUSYS_CONCURRENCY_UART_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_CONCURRENCY_UART_RX_PIN);
    printf("wire tx_pin to rx_pin before running this test\n");
    printf("iterations: %u  bytes_expected: %u\n",
           (unsigned int) BLUSYS_CONCURRENCY_UART_ITERS,
           (unsigned int) total_expected);

    err = blusys_uart_open(BLUSYS_CONCURRENCY_UART_PORT,
                           BLUSYS_CONCURRENCY_UART_TX_PIN,
                           BLUSYS_CONCURRENCY_UART_RX_PIN,
                           BLUSYS_CONCURRENCY_UART_BAUDRATE,
                           &uart);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    tx_ctx.uart = uart;
    rx_ctx.uart = uart;
    tx_ctx.report_task = xTaskGetCurrentTaskHandle();
    rx_ctx.report_task = xTaskGetCurrentTaskHandle();

    xTaskCreate(blusys_concurrency_uart_tx_worker, "uart_tx", 4096, &tx_ctx, 5, NULL);
    xTaskCreate(blusys_concurrency_uart_rx_worker, "uart_rx", 4096, &rx_ctx, 5, NULL);

    while ((notified_bits & BLUSYS_CONCURRENCY_UART_DONE_MASK) != BLUSYS_CONCURRENCY_UART_DONE_MASK) {
        xTaskNotifyWait(0u, UINT32_MAX, &notified_bits, portMAX_DELAY);
    }

    printf("tx_errors: %u\n", (unsigned int) tx_ctx.errors);
    printf("rx_errors: %u\n", (unsigned int) rx_ctx.errors);
    printf("bytes_sent: %u\n", (unsigned int) tx_ctx.bytes_transferred);
    printf("bytes_received: %u\n", (unsigned int) rx_ctx.bytes_transferred);
    printf("concurrent_result: %s\n",
           ((tx_ctx.errors == 0u) &&
            (rx_ctx.errors == 0u) &&
            (rx_ctx.bytes_transferred == total_expected)) ? "ok" : "fail");

    err = blusys_uart_close(uart);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
