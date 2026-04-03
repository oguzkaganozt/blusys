#include "blusys/uart.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/task.h"

#define BLUSYS_UART_RX_BUFFER_SIZE 256

struct blusys_uart {
    uart_port_t port;
    blusys_lock_t lock;
};

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

    err = blusys_lock_init(&uart->lock);
    if (err != BLUSYS_OK) {
        free(uart);
        return err;
    }

    uart->port = (uart_port_t) port;

    esp_err = uart_driver_install(uart->port,
                                  BLUSYS_UART_RX_BUFFER_SIZE,
                                  0,
                                  0,
                                  NULL,
                                  0);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail;
    }

    esp_err = uart_param_config(uart->port, &config);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_driver;
    }

    esp_err = uart_set_pin(uart->port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_driver;
    }

    *out_uart = uart;

    return BLUSYS_OK;

fail_driver:
    uart_driver_delete(uart->port);
fail:
    blusys_lock_deinit(&uart->lock);
    free(uart);
    return err;
}

blusys_err_t blusys_uart_close(blusys_uart_t *uart)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (uart == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&uart->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = uart_driver_delete(uart->port);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&uart->lock);

    if (err == BLUSYS_OK) {
        blusys_lock_deinit(&uart->lock);
        free(uart);
    }

    return err;
}

blusys_err_t blusys_uart_write(blusys_uart_t *uart, const void *data, size_t size, int timeout_ms)
{
    blusys_err_t err;
    const char *buffer = data;
    TickType_t start_ticks;
    size_t offset = 0u;
    int written;
    bool timed_out;

    if ((uart == NULL) || ((data == NULL) && (size != 0u)) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (size == 0u) {
        return BLUSYS_OK;
    }

    err = blusys_lock_take(&uart->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    start_ticks = xTaskGetTickCount();

    while (offset < size) {
        size_t remaining = size - offset;
        uint32_t chunk_size = (remaining > UINT32_MAX) ? UINT32_MAX : (uint32_t) remaining;

        written = uart_tx_chars(uart->port, buffer + offset, chunk_size);
        if (written < 0) {
            blusys_lock_give(&uart->lock);
            return BLUSYS_ERR_IO;
        }

        if (written > 0) {
            offset += (size_t) written;
            continue;
        }

        err = blusys_translate_esp_err(uart_wait_tx_done(
            uart->port,
            blusys_uart_timeout_remaining_ticks(start_ticks, timeout_ms, &timed_out)));
        if ((err != BLUSYS_OK) || timed_out) {
            blusys_lock_give(&uart->lock);
            return timed_out ? BLUSYS_ERR_TIMEOUT : err;
        }
    }

    err = blusys_translate_esp_err(uart_wait_tx_done(
        uart->port,
        blusys_uart_timeout_remaining_ticks(start_ticks, timeout_ms, &timed_out)));

    blusys_lock_give(&uart->lock);

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

    err = blusys_lock_take(&uart->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    read_size = uart_read_bytes(uart->port, data, (uint32_t) size, blusys_timeout_ms_to_ticks(timeout_ms));
    if (read_size < 0) {
        blusys_lock_give(&uart->lock);
        return BLUSYS_ERR_IO;
    }

    *out_read = (size_t) read_size;

    blusys_lock_give(&uart->lock);

    return (*out_read == size) ? BLUSYS_OK : BLUSYS_ERR_TIMEOUT;
}
