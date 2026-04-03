# UART

## Purpose

The `uart` module provides a blocking handle-based serial API for the common UART setup path:

- open a UART port with TX, RX, and baud rate
- write bytes with a timeout
- read bytes with a timeout
- close the UART handle

Phase 3 intentionally fixes the line format to 8 data bits, no parity, and 1 stop bit.

## Supported Targets

- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include <stddef.h>
#include <stdint.h>

#include "blusys/uart.h"

void app_main(void)
{
    blusys_uart_t *uart;
    uint8_t rx_buffer[16];
    size_t read_size;

    if (blusys_uart_open(1, 21, 20, 115200, &uart) != BLUSYS_OK) {
        return;
    }

    blusys_uart_write(uart, "ping", 4, 1000);
    blusys_uart_read(uart, rx_buffer, sizeof(rx_buffer), 1000, &read_size);
    blusys_uart_close(uart);
}
```

## Lifecycle Model

UART is handle-based:

1. call `blusys_uart_open()`
2. use `blusys_uart_write()` and `blusys_uart_read()`
3. call `blusys_uart_close()` when finished

## Blocking APIs

- `blusys_uart_open()`
- `blusys_uart_close()`
- `blusys_uart_write()`
- `blusys_uart_read()`

## Async APIs

None in Phase 3.
UART async TX and RX remain Phase 5 work.

## Thread Safety

- concurrent read and write calls on the same handle are serialized internally
- different UART handles may be used independently
- do not call `blusys_uart_close()` concurrently with other calls using the same handle

## ISR Notes

Phase 3 does not define ISR-safe UART calls.

## Limitations

- only the common blocking path is exposed in Phase 3
- the line format is fixed to 8N1
- hardware flow control is not exposed
- each Blusys UART handle owns one ESP-IDF UART driver instance for one UART port

## Error Behavior

- invalid ports, pins, baud rate, or pointers return `BLUSYS_ERR_INVALID_ARG`
- trying to open a UART port that already has a driver installed returns `BLUSYS_ERR_INVALID_STATE`
- `blusys_uart_read()` returns `BLUSYS_ERR_TIMEOUT` when fewer than the requested bytes arrive before the timeout and still reports the partial byte count through `out_read`
- `blusys_uart_write()` returns `BLUSYS_ERR_TIMEOUT` if transmit completion does not finish before the timeout

## Example App

See `examples/uart_loopback/`.
