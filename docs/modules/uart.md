# UART

## Purpose

The `uart` module provides a blocking handle-based serial API for the common UART setup path, plus a small async callback layer:

- open a UART port with TX, RX, and baud rate
- write bytes with a timeout
- read bytes with a timeout
- queue one async TX operation
- receive async RX data through task-context callbacks
- close the UART handle

Phase 3 intentionally fixes the line format to 8 data bits, no parity, and 1 stop bit.

## Supported Targets

- ESP32
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
3. optionally register async TX and RX callbacks
4. optionally call `blusys_uart_write_async()`
5. call `blusys_uart_close()` when finished

## Blocking APIs

- `blusys_uart_open()`
- `blusys_uart_close()`
- `blusys_uart_write()`
- `blusys_uart_read()`

## Async APIs

- `blusys_uart_set_tx_callback()` registers one task-context callback for async TX completion
- `blusys_uart_set_rx_callback()` registers one task-context callback for async RX data delivery
- `blusys_uart_write_async()` queues one async TX operation at a time

RX callbacks receive UART data in task context through an internal worker task. TX callbacks also run in task context after the queued write finishes or fails.

## Thread Safety

- concurrent read and write calls on the same handle are serialized internally
- different UART handles may be used independently
- do not call `blusys_uart_close()` concurrently with other calls using the same handle
- do not call `blusys_uart_close()` from a UART async callback
- do not call `blusys_uart_set_tx_callback()` or `blusys_uart_set_rx_callback()` from a UART async callback
- do not change the TX completion callback while an async TX is pending
- blocking `blusys_uart_read()` is not allowed while an RX callback is registered
- blocking and async TX cannot overlap on the same handle

## ISR Notes

UART callbacks do not run in ISR context.
The async layer uses an internal task that consumes the ESP-IDF UART event queue.

## Limitations

- only one async TX operation can be pending per handle
- RX async delivery is callback-based, not user-buffer-based
- the line format is fixed to 8N1
- hardware flow control is not exposed
- each Blusys UART handle owns one ESP-IDF UART driver instance for one UART port
- closing the handle cancels any pending async callback delivery
- `blusys_uart_close()` is not allowed from the UART async callback task
- callback registration changes are not allowed from the UART async callback task

## Error Behavior

- invalid ports, pins, baud rate, or pointers return `BLUSYS_ERR_INVALID_ARG`
- trying to open a UART port that already has a driver installed returns `BLUSYS_ERR_INVALID_STATE`
- `blusys_uart_read()` returns `BLUSYS_ERR_TIMEOUT` when fewer than the requested bytes arrive before the timeout and still reports the partial byte count through `out_read`
- `blusys_uart_write()` returns `BLUSYS_ERR_TIMEOUT` if transmit completion does not finish before the timeout
- `blusys_uart_write_async()` returns `BLUSYS_ERR_INVALID_STATE` if another async TX is already pending
- `blusys_uart_set_tx_callback()` returns `BLUSYS_ERR_INVALID_STATE` while an async TX is pending
- `blusys_uart_read()` returns `BLUSYS_ERR_INVALID_STATE` while an RX callback is registered

## Example App

See `examples/uart_loopback/`.
See `examples/uart_async/` for task-context callback usage.
