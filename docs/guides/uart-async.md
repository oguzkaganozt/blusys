# Send And Receive Over UART With Callbacks

## Problem Statement

You want non-blocking UART TX and RX using callbacks, so your task is notified when data has been sent or received rather than blocking on each call.

## Prerequisites

- a supported board
- TX and RX pins wired together (loopback) for self-testing, or connected to another device

## Minimal Example

```c
static void on_tx(blusys_uart_t *uart, blusys_err_t status, void *ctx)
{
    (void) uart;
    TaskHandle_t task = ctx;
    xTaskNotifyFromISR(task, 1u, eSetBits, NULL);
}

static void on_rx(blusys_uart_t *uart, const void *data, size_t size, void *ctx)
{
    (void) uart;
    /* copy data; notify task */
}

void app_main(void)
{
    blusys_uart_t *uart;
    TaskHandle_t self = xTaskGetCurrentTaskHandle();

    blusys_uart_open(1, 4, 5, 115200, &uart);
    blusys_uart_set_tx_callback(uart, on_tx, self);
    blusys_uart_set_rx_callback(uart, on_rx, self);

    blusys_uart_write_async(uart, "hello", 5);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  /* wait for TX done */

    blusys_uart_close(uart);
}
```

## APIs Used

- `blusys_uart_open()` configures port, TX/RX pins, and baud rate
- `blusys_uart_set_tx_callback()` registers a function called when a `write_async()` completes; replaces any previous TX callback
- `blusys_uart_set_rx_callback()` registers a function called each time data arrives; replaces any previous RX callback
- `blusys_uart_write_async()` enqueues data and returns immediately; the callback fires when the last byte has been sent
- `blusys_uart_close()` stops callbacks and releases the handle

## Callback Semantics

**TX callback** is called once per `write_async()` call, after all bytes have been transmitted. `status` is `BLUSYS_OK` on success.

**RX callback** is called each time the driver delivers a chunk of received bytes. `data` and `size` describe the chunk; the buffer is only valid for the duration of the callback. Copy data before returning.

Both callbacks run in a driver task context, not in an ISR. Task API calls (e.g. `xTaskNotify`) are safe. Do not call blocking UART functions from within a callback.

## Callback Replacement

Calling `set_tx_callback()` or `set_rx_callback()` again with a new function and context atomically replaces the previous registration. The example uses this to verify that mid-flight callback replacement works correctly.

## Expected Runtime Behavior

- `write_async()` returns immediately; the TX callback fires after all bytes leave the hardware FIFO
- RX callback fires as bytes arrive; for loopback, it fires shortly after TX completes
- two back-to-back rounds with different callbacks both complete successfully

## Common Mistakes

- accessing the `data` pointer in the RX callback after the callback returns — the buffer is not retained
- calling blocking UART functions (`write`, `read`) from inside a callback — this deadlocks

## Example App

See `examples/uart_async/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [UART API Reference](../modules/uart.md).
