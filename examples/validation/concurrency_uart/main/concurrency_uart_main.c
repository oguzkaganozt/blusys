#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

/*
 * Concurrency test: one task calls blusys_uart_write() while another calls
 * blusys_uart_read() on the same handle simultaneously.
 *
 * With TX wired to RX (loopback) the RX worker receives exactly what the TX
 * worker sends.  The library must allow concurrent TX and RX without either
 * path blocking the other — verified by the split tx_lock / rx_lock design.
 *
 * Pass criteria: bytes_received == bytes_sent, no write or read errors.
 */

#define BLUSYS_CONCURRENCY_UART_PORT     CONFIG_BLUSYS_CONCURRENCY_UART_PORT
#define BLUSYS_CONCURRENCY_UART_TX_PIN   CONFIG_BLUSYS_CONCURRENCY_UART_TX_PIN
#define BLUSYS_CONCURRENCY_UART_RX_PIN   CONFIG_BLUSYS_CONCURRENCY_UART_RX_PIN
#define BLUSYS_CONCURRENCY_UART_BAUDRATE 115200u
#define BLUSYS_CONCURRENCY_UART_ITERS    50u

/* RX worker stops after this many consecutive reads that return 0 bytes */
#define BLUSYS_CONCURRENCY_UART_RX_EMPTY_LIMIT 3u
#define BLUSYS_CONCURRENCY_UART_RX_TIMEOUT_MS  200

#define BLUSYS_CONCURRENCY_UART_TX_DONE (1u << 0)
#define BLUSYS_CONCURRENCY_UART_RX_DONE (1u << 1)
#define BLUSYS_CONCURRENCY_UART_DONE_MASK \
    (BLUSYS_CONCURRENCY_UART_TX_DONE | BLUSYS_CONCURRENCY_UART_RX_DONE)

static const char blusys_concurrency_uart_message[] = "blusys-concurrency-uart\n";

typedef struct {
    blusys_uart_t *uart;
    uint32_t bytes_sent;
    uint32_t bytes_received;
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
            printf("tx iter_%u write_error: %s\n",
                   (unsigned int) i,
                   blusys_err_string(err));
            ctx->errors += 1u;
        } else {
            ctx->bytes_sent += (uint32_t) msg_len;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

static void blusys_concurrency_uart_rx_worker(void *arg)
{
    blusys_concurrency_uart_ctx_t *ctx = arg;
    uint8_t rx_buf[64];
    size_t bytes_read;
    blusys_err_t err;
    uint32_t consecutive_empty = 0u;

    while (consecutive_empty < BLUSYS_CONCURRENCY_UART_RX_EMPTY_LIMIT) {
        bytes_read = 0u;
        err = blusys_uart_read(ctx->uart,
                               rx_buf,
                               sizeof(rx_buf),
                               BLUSYS_CONCURRENCY_UART_RX_TIMEOUT_MS,
                               &bytes_read);
        ctx->bytes_received += (uint32_t) bytes_read;

        if ((err != BLUSYS_OK) && (err != BLUSYS_ERR_TIMEOUT)) {
            printf("rx read_error: %s\n", blusys_err_string(err));
            ctx->errors += 1u;
            break;
        }

        if (bytes_read == 0u) {
            consecutive_empty += 1u;
        } else {
            consecutive_empty = 0u;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

void app_main(void)
{
    blusys_concurrency_uart_ctx_t ctx_tx = {
        .uart = NULL,
        .bytes_sent = 0u,
        .bytes_received = 0u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = BLUSYS_CONCURRENCY_UART_TX_DONE,
    };
    blusys_concurrency_uart_ctx_t ctx_rx = {
        .uart = NULL,
        .bytes_sent = 0u,
        .bytes_received = 0u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = BLUSYS_CONCURRENCY_UART_RX_DONE,
    };
    blusys_uart_t *uart = NULL;
    blusys_err_t err;
    uint32_t notified_bits = 0u;
    uint32_t received_bits = 0u;

    printf("Blusys concurrency uart\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_CONCURRENCY_UART_PORT);
    printf("tx_pin: %d\n", BLUSYS_CONCURRENCY_UART_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_CONCURRENCY_UART_RX_PIN);
    printf("wire tx_pin to rx_pin before running this test\n");
    printf("iterations: %u\n", (unsigned int) BLUSYS_CONCURRENCY_UART_ITERS);

    err = blusys_uart_open(BLUSYS_CONCURRENCY_UART_PORT,
                           BLUSYS_CONCURRENCY_UART_TX_PIN,
                           BLUSYS_CONCURRENCY_UART_RX_PIN,
                           BLUSYS_CONCURRENCY_UART_BAUDRATE,
                           &uart);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    ctx_tx.uart = uart;
    ctx_rx.uart = uart;
    ctx_tx.report_task = xTaskGetCurrentTaskHandle();
    ctx_rx.report_task = xTaskGetCurrentTaskHandle();

    xTaskCreate(blusys_concurrency_uart_tx_worker, "uart_tx", 4096, &ctx_tx, 5, NULL);
    xTaskCreate(blusys_concurrency_uart_rx_worker, "uart_rx", 4096, &ctx_rx, 5, NULL);

    while ((notified_bits & BLUSYS_CONCURRENCY_UART_DONE_MASK) != BLUSYS_CONCURRENCY_UART_DONE_MASK) {
        xTaskNotifyWait(0u, UINT32_MAX, &received_bits, portMAX_DELAY);
        notified_bits |= received_bits;
    }

    printf("tx errors: %u\n", (unsigned int) ctx_tx.errors);
    printf("rx errors: %u\n", (unsigned int) ctx_rx.errors);
    printf("bytes_sent: %u\n", (unsigned int) ctx_tx.bytes_sent);
    printf("bytes_received: %u\n", (unsigned int) ctx_rx.bytes_received);
    printf("concurrent_result: %s\n",
           ((ctx_tx.errors == 0u) &&
            (ctx_rx.errors == 0u) &&
            (ctx_rx.bytes_received == ctx_tx.bytes_sent)) ? "ok" : "fail");

    err = blusys_uart_close(uart);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
