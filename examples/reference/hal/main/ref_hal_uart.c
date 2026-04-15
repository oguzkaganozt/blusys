#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#if CONFIG_REF_HAL_REFERENCE_SCENARIO_UART_BASIC

#define BLUSYS_UART_BASIC_PORT   CONFIG_BLUSYS_UART_BASIC_PORT
#define BLUSYS_UART_BASIC_TX_PIN CONFIG_BLUSYS_UART_BASIC_TX_PIN
#define BLUSYS_UART_BASIC_RX_PIN CONFIG_BLUSYS_UART_BASIC_RX_PIN
#define BLUSYS_UART_BASIC_BAUDRATE 115200u

#if CONFIG_BLUSYS_UART_BASIC_MODE_LOOPBACK

#define BLUSYS_UART_LOOPBACK_TIMEOUT_MS 1000

static void run_uart_loopback(void)
{
    static const char tx_message[] = "blusys-uart-loopback";
    char rx_buffer[sizeof(tx_message)] = {0};
    blusys_uart_t *uart = NULL;
    blusys_err_t err;
    size_t read_size = 0u;

    printf("Blusys uart_basic (loopback)\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_UART_BASIC_PORT);
    printf("tx_pin: %d\n", BLUSYS_UART_BASIC_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_UART_BASIC_RX_PIN);
    printf("wire tx_pin to rx_pin before running this example\n");

    err = blusys_uart_open(BLUSYS_UART_BASIC_PORT,
                           BLUSYS_UART_BASIC_TX_PIN,
                           BLUSYS_UART_BASIC_RX_PIN,
                           BLUSYS_UART_BASIC_BAUDRATE,
                           &uart);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_uart_write(uart,
                            tx_message,
                            sizeof(tx_message) - 1u,
                            BLUSYS_UART_LOOPBACK_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        printf("write error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    err = blusys_uart_read(uart,
                           rx_buffer,
                           sizeof(tx_message) - 1u,
                           BLUSYS_UART_LOOPBACK_TIMEOUT_MS,
                           &read_size);
    if ((err != BLUSYS_OK) && (err != BLUSYS_ERR_TIMEOUT)) {
        printf("read error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    printf("tx: %s\n", tx_message);
    printf("rx_bytes: %zu\n", read_size);
    printf("rx: %.*s\n", (int) read_size, rx_buffer);
    if ((read_size == (sizeof(tx_message) - 1u)) && (memcmp(tx_message, rx_buffer, read_size) == 0)) {
        printf("loopback_result: ok\n");
    } else {
        printf("loopback_result: incomplete\n");
    }

    err = blusys_uart_close(uart);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}

#elif CONFIG_BLUSYS_UART_BASIC_MODE_ASYNC

#define BLUSYS_UART_ASYNC_PORT     BLUSYS_UART_BASIC_PORT
#define BLUSYS_UART_ASYNC_TX_PIN   BLUSYS_UART_BASIC_TX_PIN
#define BLUSYS_UART_ASYNC_RX_PIN   BLUSYS_UART_BASIC_RX_PIN
#define BLUSYS_UART_ASYNC_TX_DONE_BIT (1u << 0)
#define BLUSYS_UART_ASYNC_RX_DONE_BIT (1u << 1)

typedef struct {
    TaskHandle_t task;
    char rx_buffer[64];
    size_t rx_size;
    size_t expected_rx_size;
    blusys_err_t tx_status;
    uint32_t notify_mask;
} blusys_uart_async_ctx_t;

static void blusys_uart_async_on_tx(blusys_uart_t *uart, blusys_err_t status, void *user_ctx)
{
    blusys_uart_async_ctx_t *ctx = user_ctx;
    (void) uart;
    ctx->tx_status = status;
    xTaskNotify(ctx->task, ctx->notify_mask | BLUSYS_UART_ASYNC_TX_DONE_BIT, eSetBits);
}

static void blusys_uart_async_on_rx(blusys_uart_t *uart, const void *data, size_t size, void *user_ctx)
{
    blusys_uart_async_ctx_t *ctx = user_ctx;
    size_t available_size = (sizeof(ctx->rx_buffer) - 1u) - ctx->rx_size;
    size_t copy_size = (size > available_size) ? available_size : size;

    (void) uart;

    if (copy_size > 0u) {
        memcpy(&ctx->rx_buffer[ctx->rx_size], data, copy_size);
        ctx->rx_size += copy_size;
        ctx->rx_buffer[ctx->rx_size] = '\0';
    }

    if (ctx->rx_size >= ctx->expected_rx_size) {
        xTaskNotify(ctx->task, ctx->notify_mask | BLUSYS_UART_ASYNC_RX_DONE_BIT, eSetBits);
    }
}

static bool blusys_uart_async_run_round(blusys_uart_t *uart,
                                        const char *label,
                                        const char *tx_message,
                                        blusys_uart_async_ctx_t *ctx)
{
    blusys_err_t err;
    uint32_t expected_bits = ctx->notify_mask | BLUSYS_UART_ASYNC_TX_DONE_BIT | BLUSYS_UART_ASYNC_RX_DONE_BIT;
    uint32_t notified_bits = 0u;
    uint32_t received_bits = 0u;

    ctx->rx_buffer[0] = '\0';
    ctx->rx_size = 0u;
    ctx->expected_rx_size = strlen(tx_message);
    ctx->tx_status = BLUSYS_OK;
    xTaskNotifyStateClear(ctx->task);

    err = blusys_uart_write_async(uart, tx_message, strlen(tx_message));
    if (err != BLUSYS_OK) {
        printf("%s write async error: %s\n", label, blusys_err_string(err));
        return false;
    }

    while ((notified_bits & expected_bits) != expected_bits) {
        xTaskNotifyWait(0u, UINT32_MAX, &received_bits, portMAX_DELAY);
        notified_bits |= received_bits;
    }

    printf("%s tx_status: %s\n", label, blusys_err_string(ctx->tx_status));
    printf("%s tx: %s\n", label, tx_message);
    printf("%s rx_bytes: %u\n", label, (unsigned int) ctx->rx_size);
    printf("%s rx: %.*s\n", label, (int) ctx->rx_size, ctx->rx_buffer);

    if ((ctx->tx_status != BLUSYS_OK) ||
        (ctx->rx_size != strlen(tx_message)) ||
        (memcmp(ctx->rx_buffer, tx_message, strlen(tx_message)) != 0)) {
        printf("%s result: incomplete\n", label);
        return false;
    }

    printf("%s result: ok\n", label);
    return true;
}

static void run_uart_async(void)
{
    static const char tx_message_a[] = "blusys-uart-async-a";
    static const char tx_message_b[] = "blusys-uart-async-b";
    blusys_uart_async_ctx_t ctx_a = {
        .task = NULL,
        .rx_buffer = {0},
        .rx_size = 0u,
        .expected_rx_size = 0u,
        .tx_status = BLUSYS_OK,
        .notify_mask = 0u,
    };
    blusys_uart_async_ctx_t ctx_b = {
        .task = NULL,
        .rx_buffer = {0},
        .rx_size = 0u,
        .expected_rx_size = 0u,
        .tx_status = BLUSYS_OK,
        .notify_mask = (1u << 4),
    };
    blusys_uart_t *uart = NULL;
    blusys_err_t err;
    bool phase_a_ok;
    bool phase_b_ok;

    ctx_a.task = xTaskGetCurrentTaskHandle();
    ctx_b.task = ctx_a.task;

    printf("Blusys uart_basic (async)\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_UART_ASYNC_PORT);
    printf("tx_pin: %d\n", BLUSYS_UART_ASYNC_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_UART_ASYNC_RX_PIN);
    printf("wire tx_pin to rx_pin before running this example\n");

    err = blusys_uart_open(BLUSYS_UART_ASYNC_PORT,
                           BLUSYS_UART_ASYNC_TX_PIN,
                           BLUSYS_UART_ASYNC_RX_PIN,
                           BLUSYS_UART_BASIC_BAUDRATE,
                           &uart);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_uart_set_tx_callback(uart, blusys_uart_async_on_tx, &ctx_a);
    if (err != BLUSYS_OK) {
        printf("set tx callback error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    err = blusys_uart_set_rx_callback(uart, blusys_uart_async_on_rx, &ctx_a);
    if (err != BLUSYS_OK) {
        printf("set rx callback error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    phase_a_ok = blusys_uart_async_run_round(uart, "phase_a", tx_message_a, &ctx_a);

    err = blusys_uart_set_tx_callback(uart, blusys_uart_async_on_tx, &ctx_b);
    if (err != BLUSYS_OK) {
        printf("replace tx callback error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    err = blusys_uart_set_rx_callback(uart, blusys_uart_async_on_rx, &ctx_b);
    if (err != BLUSYS_OK) {
        printf("replace rx callback error: %s\n", blusys_err_string(err));
        blusys_uart_close(uart);
        return;
    }

    phase_b_ok = blusys_uart_async_run_round(uart, "phase_b", tx_message_b, &ctx_b);

    printf("callback_swap_result: %s\n", (phase_a_ok && phase_b_ok) ? "ok" : "incomplete");

    err = blusys_uart_close(uart);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}

#endif

void run_uart_basic(void)
{
#if CONFIG_BLUSYS_UART_BASIC_MODE_LOOPBACK
    run_uart_loopback();
#elif CONFIG_BLUSYS_UART_BASIC_MODE_ASYNC
    run_uart_async();
#endif
}

#else /* !CONFIG_REF_HAL_REFERENCE_SCENARIO_UART_BASIC */

void run_uart_basic(void) {}

#endif /* CONFIG_REF_HAL_REFERENCE_SCENARIO_UART_BASIC */
