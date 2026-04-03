#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_UART_LOOPBACK_PORT CONFIG_BLUSYS_UART_LOOPBACK_PORT
#define BLUSYS_UART_LOOPBACK_TX_PIN CONFIG_BLUSYS_UART_LOOPBACK_TX_PIN
#define BLUSYS_UART_LOOPBACK_RX_PIN CONFIG_BLUSYS_UART_LOOPBACK_RX_PIN

#define BLUSYS_UART_LOOPBACK_BAUDRATE 115200u
#define BLUSYS_UART_LOOPBACK_TIMEOUT_MS 1000

void app_main(void)
{
    static const char tx_message[] = "blusys-uart-loopback";
    char rx_buffer[sizeof(tx_message)] = {0};
    blusys_uart_t *uart = NULL;
    blusys_err_t err;
    size_t read_size = 0u;

    printf("Blusys uart loopback\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_UART_LOOPBACK_PORT);
    printf("tx_pin: %d\n", BLUSYS_UART_LOOPBACK_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_UART_LOOPBACK_RX_PIN);
    printf("wire tx_pin to rx_pin before running this example\n");
    printf("set board-safe UART pins in menuconfig if these defaults do not match your board\n");

    err = blusys_uart_open(BLUSYS_UART_LOOPBACK_PORT,
                           BLUSYS_UART_LOOPBACK_TX_PIN,
                           BLUSYS_UART_LOOPBACK_RX_PIN,
                           BLUSYS_UART_LOOPBACK_BAUDRATE,
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
