# UART

Blocking serial communication with an optional async callback layer. Fixed line format: 8N1.

Callbacks run in task context through an internal worker task — not in ISR context.

## Quick Example

```c
blusys_uart_t *uart;
size_t read_size;
char rx_buffer[4];

blusys_uart_open(1, 21, 20, 115200, &uart);
blusys_uart_write(uart, "ping", 4, 1000);
blusys_uart_read(uart, rx_buffer, sizeof(rx_buffer), 1000, &read_size);
blusys_uart_close(uart);
```

## Common Mistakes

- using a UART port that is already occupied by board firmware or application code
- forgetting to connect TX to RX for loopback testing
- using board pins that are not routed safely on your hardware
- calling `blusys_uart_read()` while an RX callback is registered (mutually exclusive)
- calling `blusys_uart_close()` from inside a UART async callback

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- concurrent read and write on the same handle are serialized internally
- different handles may be used independently
- do not call `blusys_uart_close()` concurrently with other calls on the same handle
- do not call `blusys_uart_close()` or change callbacks from within a UART async callback
- blocking `blusys_uart_read()` must not overlap with an active RX callback
- blocking and async TX must not overlap on the same handle

## ISR Notes

UART callbacks run in task context through an internal worker task, not in ISR context.

## Limitations

- one async TX operation can be pending per handle
- the line format is fixed to 8N1; hardware flow control is not exposed
- each handle owns one ESP-IDF UART driver instance for one UART port
- closing the handle cancels pending async callback delivery

## Example App

See `examples/reference/hal/` (UART scenario — blocking loopback) and `examples/validation/platform_lab/` (UART suite — async callbacks).
