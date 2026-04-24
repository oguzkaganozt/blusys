# UART

Blocking serial communication with an optional async callback layer.

## At a glance

- fixed 8N1 line format
- callbacks run in task context
- concurrent read/write on the same handle is serialised

## Quick example

```c
blusys_uart_t *uart;
size_t read_size;
char rx_buffer[4];

blusys_uart_open(1, 21, 20, 115200, &uart);
blusys_uart_write(uart, "ping", 4, 1000);
blusys_uart_read(uart, rx_buffer, sizeof(rx_buffer), 1000, &read_size);
blusys_uart_close(uart);
```

## Common mistakes

- using a busy port
- forgetting loopback for tests
- overlapping blocking reads with RX callbacks
- closing from inside a callback

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- different handles may be used independently
- blocking and async TX must not overlap on the same handle

## ISR notes

- callbacks run in task context, not ISR context

## Limitations

- one async TX can be pending per handle
- hardware flow control is not exposed

## Example apps

- `examples/reference/hal` (`UART`)
- `examples/validation/platform_lab` (`UART callbacks`)
