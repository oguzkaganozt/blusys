# UART Loopback

## Problem Statement

You want to confirm that a UART port can transmit and receive bytes through the Blusys Phase 3 API.

## Prerequisites

- a supported board
- a jumper wire from the example TX pin to the example RX pin
- example UART pins reviewed in `idf.py menuconfig` if your board uses different safe pins

## Minimal Example

```c
blusys_uart_t *uart;
size_t read_size;
char rx_buffer[4];

blusys_uart_open(1, 21, 20, 115200, &uart);
blusys_uart_write(uart, "ping", 4, 1000);
blusys_uart_read(uart, rx_buffer, sizeof(rx_buffer), 1000, &read_size);
blusys_uart_close(uart);
```

## APIs Used

- `blusys_uart_open()` configures the UART port with TX, RX, and baud rate
- `blusys_uart_write()` sends bytes and waits for transmit completion
- `blusys_uart_read()` waits for incoming bytes and reports how many arrived
- `blusys_uart_flush_rx()` clears buffered RX data before a fresh blocking read when needed
- `blusys_uart_close()` releases the driver instance

## Expected Runtime Behavior

- the example prints the configured UART port and pins
- after the TX and RX pins are looped together, the received bytes should match the transmitted message
- if fewer bytes arrive before the timeout, the example reports an incomplete result instead of crashing

## Common Mistakes

- using a UART port that is already occupied by board firmware or application code
- forgetting to connect TX to RX for loopback testing
- using board pins that are not routed safely on your hardware

## Example App

See `examples/uart_loopback/` for a runnable example.
Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
