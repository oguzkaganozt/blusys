#include "blusys/hal/uart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "blusys/hal/internal/callback_lifecycle.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define BLUSYS_UART_RX_BUFFER_SIZE 256
#define BLUSYS_UART_TX_BUFFER_SIZE 256
#define BLUSYS_UART_EVENT_QUEUE_SIZE 16
#define BLUSYS_UART_EVENT_TASK_STACK_SIZE 4096
#define BLUSYS_UART_EVENT_POLL_TICKS pdMS_TO_TICKS(20)
#define BLUSYS_UART_RX_CHUNK_SIZE 64
#define UART_DOMAIN err_domain_hal_uart

struct blusys_uart {
    uart_port_t port;
    blusys_lock_t tx_lock;
    blusys_lock_t rx_lock;
    QueueHandle_t event_queue;
    TaskHandle_t event_task;
    blusys_uart_tx_callback_t tx_callback;
    void *tx_callback_user_ctx;
    blusys_uart_rx_callback_t rx_callback;
    void *rx_callback_user_ctx;
    unsigned int tx_callback_active;
    unsigned int rx_callback_active;
    bool tx_pending;
    bool closing;
    bool event_task_exited;
};

static void blusys_uart_event_task(void *arg);

static void blusys_uart_tx_callback_finish(blusys_uart_t *uart)
{
    blusys_err_t err;

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return;
    }

    blusys_callback_lifecycle_leave(&uart->tx_callback_active);

    blusys_lock_give(&uart->tx_lock);
}

static void blusys_uart_rx_callback_finish(blusys_uart_t *uart)
{
    blusys_err_t err;

    err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return;
    }

    blusys_callback_lifecycle_leave(&uart->rx_callback_active);

    blusys_lock_give(&uart->rx_lock);
}

static bool blusys_uart_is_valid_port(int port)
{
    return (port >= 0) && (port < UART_NUM_MAX);
}

static bool blusys_uart_is_valid_tx_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_uart_is_valid_rx_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

static TickType_t blusys_uart_timeout_remaining_ticks(TickType_t start_ticks, int timeout_ms, bool *out_timed_out)
{
    TickType_t elapsed_ticks;
    TickType_t timeout_ticks;

    if (timeout_ms == BLUSYS_TIMEOUT_FOREVER) {
        *out_timed_out = false;
        return portMAX_DELAY;
    }

    timeout_ticks = blusys_timeout_ms_to_ticks(timeout_ms);
    elapsed_ticks = xTaskGetTickCount() - start_ticks;
    if (elapsed_ticks >= timeout_ticks) {
        *out_timed_out = true;
        return 0;
    }

    *out_timed_out = false;
    return timeout_ticks - elapsed_ticks;
}

static bool blusys_uart_snapshot_tx_completion(blusys_uart_t *uart,
                                               blusys_uart_tx_callback_t *out_callback,
                                               void **out_user_ctx,
                                               blusys_err_t *out_status)
{
    blusys_err_t err;
    esp_err_t esp_err;

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        *out_status = err;
        *out_callback = NULL;
        *out_user_ctx = NULL;
        return true;
    }

    if (!uart->tx_pending) {
        blusys_lock_give(&uart->tx_lock);
        return false;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing)) {
        *out_status = BLUSYS_ERR_INVALID_STATE;
        *out_callback = NULL;
        *out_user_ctx = NULL;
        uart->tx_pending = false;
        blusys_lock_give(&uart->tx_lock);
        return true;
    }

    esp_err = uart_wait_tx_done(uart->port, 0);
    if (esp_err == ESP_ERR_TIMEOUT) {
        blusys_lock_give(&uart->tx_lock);
        return false;
    }

    *out_status = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);
    *out_callback = uart->tx_callback;
    *out_user_ctx = uart->tx_callback_user_ctx;
    if (*out_callback != NULL) {
        blusys_callback_lifecycle_try_enter(&uart->tx_callback_active, &uart->closing);
    }
    uart->tx_pending = false;

    blusys_lock_give(&uart->tx_lock);
    return true;
}

static void blusys_uart_dispatch_rx_data(blusys_uart_t *uart, size_t pending_size)
{
    uint8_t buffer[BLUSYS_UART_RX_CHUNK_SIZE];
    size_t remaining = pending_size;

    while (remaining > 0u) {
        blusys_uart_rx_callback_t callback;
        void *callback_user_ctx;
        blusys_err_t err;
        int read_size;
        size_t chunk_size = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;

        err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return;
        }

        if (blusys_callback_lifecycle_is_closing(&uart->closing) || (uart->rx_callback == NULL)) {
            blusys_lock_give(&uart->rx_lock);
            return;
        }

        callback = uart->rx_callback;
        callback_user_ctx = uart->rx_callback_user_ctx;
        if (!blusys_callback_lifecycle_try_enter(&uart->rx_callback_active, &uart->closing)) {
            blusys_lock_give(&uart->rx_lock);
            return;
        }
        blusys_lock_give(&uart->rx_lock);

        read_size = uart_read_bytes(uart->port, buffer, (uint32_t) chunk_size, 0);
        if (read_size <= 0) {
            blusys_uart_rx_callback_finish(uart);
            return;
        }

        if (callback != NULL) {
            callback(uart, buffer, (size_t) read_size, callback_user_ctx);
        }
        blusys_uart_rx_callback_finish(uart);
        remaining -= (size_t) read_size;
    }
}

static void blusys_uart_handle_event(blusys_uart_t *uart, const uart_event_t *event)
{
    blusys_uart_rx_callback_t callback;
    void *callback_user_ctx;
    blusys_err_t err;

    switch (event->type) {
    case UART_DATA:
        err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return;
        }

        callback = uart->rx_callback;
        callback_user_ctx = uart->rx_callback_user_ctx;
        blusys_lock_give(&uart->rx_lock);

        if (callback == NULL) {
            (void) callback_user_ctx;
            return;
        }

        blusys_uart_dispatch_rx_data(uart, event->size);
        return;

    case UART_FIFO_OVF:
    case UART_BUFFER_FULL:
        uart_flush_input(uart->port);
        return;

    default:
        return;
    }
}

static void blusys_uart_event_task(void *arg)
{
    blusys_uart_t *uart = arg;

    while (true) {
        uart_event_t event;
        BaseType_t queue_result;
        blusys_uart_tx_callback_t tx_callback;
        void *tx_callback_user_ctx;
        blusys_err_t tx_status;
        blusys_err_t err;

        err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            continue;
        }

        if (blusys_callback_lifecycle_is_closing(&uart->closing)) {
            uart->event_task_exited = true;
            blusys_lock_give(&uart->tx_lock);
            vTaskDelete(NULL);
        }

        blusys_lock_give(&uart->tx_lock);

        queue_result = xQueueReceive(uart->event_queue, &event, BLUSYS_UART_EVENT_POLL_TICKS);
        if (queue_result == pdTRUE) {
            blusys_uart_handle_event(uart, &event);
        }

        if (blusys_uart_snapshot_tx_completion(uart, &tx_callback, &tx_callback_user_ctx, &tx_status)) {
            if (tx_callback != NULL) {
                tx_callback(uart, tx_status, tx_callback_user_ctx);
            }
            blusys_uart_tx_callback_finish(uart);
        }
    }
}

blusys_err_t blusys_uart_open(int port,
                              int tx_pin,
                              int rx_pin,
                              uint32_t baudrate,
                              blusys_uart_t **out_uart)
{
    uart_config_t config = {
        .baud_rate = (int) baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };
    blusys_uart_t *uart;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((!blusys_uart_is_valid_port(port)) ||
        (!blusys_uart_is_valid_tx_pin(tx_pin)) ||
        (!blusys_uart_is_valid_rx_pin(rx_pin)) ||
        (baudrate == 0u) ||
        (out_uart == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (uart_is_driver_installed((uart_port_t) port)) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    uart = calloc(1, sizeof(*uart));
    if (uart == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&uart->tx_lock);
    if (err != BLUSYS_OK) {
        free(uart);
        return err;
    }

    err = blusys_lock_init(&uart->rx_lock);
    if (err != BLUSYS_OK) {
        blusys_lock_deinit(&uart->tx_lock);
        free(uart);
        return err;
    }

    uart->port = (uart_port_t) port;

    esp_err = uart_driver_install(uart->port,
                                  BLUSYS_UART_RX_BUFFER_SIZE,
                                  BLUSYS_UART_TX_BUFFER_SIZE,
                                  BLUSYS_UART_EVENT_QUEUE_SIZE,
                                  &uart->event_queue,
                                  0);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);
        goto fail;
    }

    esp_err = uart_param_config(uart->port, &config);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);
        goto fail_driver;
    }

    esp_err = uart_set_pin(uart->port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);
        goto fail_driver;
    }

    esp_err = uart_set_rx_timeout(uart->port, 1u);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);
        goto fail_driver;
    }

    esp_err = uart_set_rx_full_threshold(uart->port, 1);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);
        goto fail_driver;
    }

    if (xTaskCreate(blusys_uart_event_task,
                    "blusys_uart_evt",
                    BLUSYS_UART_EVENT_TASK_STACK_SIZE,
                    uart,
                    tskIDLE_PRIORITY + 1,
                    &uart->event_task) != pdPASS) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_driver;
    }

    *out_uart = uart;

    return BLUSYS_OK;

fail_driver:
    uart_driver_delete(uart->port);
fail:
    blusys_lock_deinit(&uart->rx_lock);
    blusys_lock_deinit(&uart->tx_lock);
    free(uart);
    return err;
}

blusys_err_t blusys_uart_close(blusys_uart_t *uart)
{
    blusys_err_t err;
    esp_err_t esp_err;
    bool event_task_exited;

    if (uart == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (xTaskGetCurrentTaskHandle() == uart->event_task) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_callback_lifecycle_set_closing(&uart->closing, true);
    uart->tx_pending = false;
    uart->tx_callback = NULL;
    uart->tx_callback_user_ctx = NULL;
    blusys_lock_give(&uart->tx_lock);

    err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    uart->rx_callback = NULL;
    uart->rx_callback_user_ctx = NULL;
    blusys_lock_give(&uart->rx_lock);

    do {
        vTaskDelay(1);

        err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return err;
        }

        event_task_exited = uart->event_task_exited;
        blusys_lock_give(&uart->tx_lock);
    } while (!event_task_exited);

    blusys_callback_lifecycle_wait_for_idle(&uart->tx_callback_active);
    blusys_callback_lifecycle_wait_for_idle(&uart->rx_callback_active);

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&uart->tx_lock);
        return err;
    }

    esp_err = uart_driver_delete(uart->port);
    err = blusys_translate_esp_err_in(UART_DOMAIN, esp_err);

    blusys_lock_give(&uart->rx_lock);
    blusys_lock_give(&uart->tx_lock);

    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_lock_deinit(&uart->rx_lock);
    blusys_lock_deinit(&uart->tx_lock);
    free(uart);

    return BLUSYS_OK;
}

blusys_err_t blusys_uart_write(blusys_uart_t *uart, const void *data, size_t size, int timeout_ms)
{
    blusys_err_t err;
    TickType_t start_ticks;
    int written;
    bool timed_out;

    if ((uart == NULL) || ((data == NULL) && (size != 0u)) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing) || uart->tx_pending) {
        blusys_lock_give(&uart->tx_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    start_ticks = xTaskGetTickCount();

    written = uart_write_bytes(uart->port, data, size);
    if (written < 0) {
        blusys_lock_give(&uart->tx_lock);
        return BLUSYS_ERR_IO;
    }

    err = blusys_translate_esp_err_in(UART_DOMAIN, uart_wait_tx_done(
        uart->port,
        blusys_uart_timeout_remaining_ticks(start_ticks, timeout_ms, &timed_out)));

    blusys_lock_give(&uart->tx_lock);

    return timed_out ? BLUSYS_ERR_TIMEOUT : err;
}

blusys_err_t blusys_uart_read(blusys_uart_t *uart,
                              void *data,
                              size_t size,
                              int timeout_ms,
                              size_t *out_read)
{
    blusys_err_t err;
    int read_size;

    if ((uart == NULL) ||
        ((data == NULL) && (size != 0u)) ||
        ((out_read == NULL) && (size != 0u)) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        if (out_read != NULL) {
            *out_read = 0u;
        }
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing) || (uart->rx_callback != NULL)) {
        blusys_lock_give(&uart->rx_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    read_size = uart_read_bytes(uart->port, data, (uint32_t) size, blusys_timeout_ms_to_ticks(timeout_ms));
    if (read_size < 0) {
        blusys_lock_give(&uart->rx_lock);
        return BLUSYS_ERR_IO;
    }

    *out_read = (size_t) read_size;

    blusys_lock_give(&uart->rx_lock);

    return (*out_read == size) ? BLUSYS_OK : BLUSYS_ERR_TIMEOUT;
}

blusys_err_t blusys_uart_flush_rx(blusys_uart_t *uart)
{
    blusys_err_t err;

    if (uart == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing) || (uart->rx_callback != NULL)) {
        blusys_lock_give(&uart->rx_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err_in(UART_DOMAIN, uart_flush_input(uart->port));

    blusys_lock_give(&uart->rx_lock);

    return err;
}

blusys_err_t blusys_uart_set_tx_callback(blusys_uart_t *uart,
                                         blusys_uart_tx_callback_t callback,
                                         void *user_ctx)
{
    blusys_err_t err;
    bool from_event_task;
    bool wait_for_idle = false;

    if (uart == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing) || uart->tx_pending) {
        blusys_lock_give(&uart->tx_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    from_event_task = xTaskGetCurrentTaskHandle() == uart->event_task;

    if ((uart->tx_callback != NULL) && ((uart->tx_callback != callback) || (uart->tx_callback_user_ctx != user_ctx))) {
        if (from_event_task) {
            blusys_lock_give(&uart->tx_lock);
            return BLUSYS_ERR_INVALID_STATE;
        }

        uart->tx_callback = NULL;
        uart->tx_callback_user_ctx = NULL;
        wait_for_idle = true;
    } else if (uart->tx_callback == callback) {
        uart->tx_callback_user_ctx = user_ctx;
        blusys_lock_give(&uart->tx_lock);
        return BLUSYS_OK;
    }

    blusys_lock_give(&uart->tx_lock);

    if (wait_for_idle) {
        blusys_callback_lifecycle_wait_for_idle(&uart->tx_callback_active);

        err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return err;
        }

        if (blusys_callback_lifecycle_is_closing(&uart->closing) || uart->tx_pending) {
            blusys_lock_give(&uart->tx_lock);
            return BLUSYS_ERR_INVALID_STATE;
        }
    } else {
        err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return err;
        }
    }

    uart->tx_callback = callback;
    uart->tx_callback_user_ctx = user_ctx;

    blusys_lock_give(&uart->tx_lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_set_rx_callback(blusys_uart_t *uart,
                                         blusys_uart_rx_callback_t callback,
                                         void *user_ctx)
{
    blusys_err_t err;
    bool from_event_task;
    bool wait_for_idle = false;

    if (uart == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing)) {
        blusys_lock_give(&uart->rx_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    from_event_task = xTaskGetCurrentTaskHandle() == uart->event_task;

    if ((uart->rx_callback != NULL) && ((uart->rx_callback != callback) || (uart->rx_callback_user_ctx != user_ctx))) {
        if (from_event_task) {
            blusys_lock_give(&uart->rx_lock);
            return BLUSYS_ERR_INVALID_STATE;
        }

        uart->rx_callback = NULL;
        uart->rx_callback_user_ctx = NULL;
        wait_for_idle = true;
    } else if (uart->rx_callback == callback) {
        uart->rx_callback_user_ctx = user_ctx;
        if (callback != NULL) {
            uart_flush_input(uart->port);
        }
        blusys_lock_give(&uart->rx_lock);
        return BLUSYS_OK;
    }

    blusys_lock_give(&uart->rx_lock);

    if (wait_for_idle) {
        blusys_callback_lifecycle_wait_for_idle(&uart->rx_callback_active);

        err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return err;
        }

        if (blusys_callback_lifecycle_is_closing(&uart->closing)) {
            blusys_lock_give(&uart->rx_lock);
            return BLUSYS_ERR_INVALID_STATE;
        }
    } else {
        err = blusys_lock_take(&uart->rx_lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            return err;
        }
    }

    uart->rx_callback = callback;
    uart->rx_callback_user_ctx = user_ctx;

    if (callback != NULL) {
        uart_flush_input(uart->port);
    }

    blusys_lock_give(&uart->rx_lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_uart_write_async(blusys_uart_t *uart, const void *data, size_t size)
{
    blusys_err_t err;
    int written;

    if ((uart == NULL) || ((data == NULL) && (size != 0u))) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&uart->tx_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_callback_lifecycle_is_closing(&uart->closing) || uart->tx_pending) {
        blusys_lock_give(&uart->tx_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    written = uart_write_bytes(uart->port, data, size);
    if (written < 0) {
        blusys_lock_give(&uart->tx_lock);
        return BLUSYS_ERR_IO;
    }

    uart->tx_pending = true;

    blusys_lock_give(&uart->tx_lock);
    return BLUSYS_OK;
}
