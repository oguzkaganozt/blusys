# UART

Blocking serial communication with an optional async callback layer. Fixed line format: 8N1.

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

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_uart_t`

```c
typedef struct blusys_uart blusys_uart_t;
```

Opaque handle returned by `blusys_uart_open()`.

### `blusys_uart_tx_callback_t`

```c
typedef void (*blusys_uart_tx_callback_t)(blusys_uart_t *uart, blusys_err_t status, void *user_ctx);
```

Called in task context when an async TX operation completes or fails.

### `blusys_uart_rx_callback_t`

```c
typedef void (*blusys_uart_rx_callback_t)(blusys_uart_t *uart,
                                          const void *data,
                                          size_t size,
                                          void *user_ctx);
```

Called in task context when received data is available. The data pointer is valid only for the duration of the callback.

## Functions

### `blusys_uart_open`

```c
blusys_err_t blusys_uart_open(int port,
                              int tx_pin,
                              int rx_pin,
                              uint32_t baudrate,
                              blusys_uart_t **out_uart);
```

Opens a UART port and installs the driver. Line format is fixed to 8N1.

**Parameters:**
- `port` — UART port number (0, 1, or 2 depending on target)
- `tx_pin` — GPIO number for TX
- `rx_pin` — GPIO number for RX
- `baudrate` — baud rate in bits per second
- `out_uart` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments, `BLUSYS_ERR_INVALID_STATE` if the port already has a driver installed.

---

### `blusys_uart_close`

```c
blusys_err_t blusys_uart_close(blusys_uart_t *uart);
```

Stops any pending async operation, uninstalls the driver, and frees the handle. Must not be called from within a UART async callback.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `uart` is NULL, `BLUSYS_ERR_INVALID_STATE` if called from within a UART callback.

---

### `blusys_uart_write`

```c
blusys_err_t blusys_uart_write(blusys_uart_t *uart, const void *data, size_t size, int timeout_ms);
```

Writes bytes to the UART. Blocks until all bytes are transmitted or the timeout expires.

**Parameters:**
- `uart` — handle
- `data` — buffer to send
- `size` — number of bytes
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if transmit does not complete before the timeout.

---

### `blusys_uart_read`

```c
blusys_err_t blusys_uart_read(blusys_uart_t *uart,
                              void *data,
                              size_t size,
                              int timeout_ms,
                              size_t *out_read);
```

Reads up to `size` bytes. Blocks until at least one byte is received or the timeout expires. Partial reads are reported through `out_read`.

**Parameters:**
- `uart` — handle
- `data` — receive buffer
- `size` — maximum bytes to read
- `timeout_ms` — milliseconds to wait
- `out_read` — number of bytes actually received

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` when fewer than `size` bytes arrive before the timeout (partial count still reported), `BLUSYS_ERR_INVALID_STATE` while an RX callback is registered.

---

### `blusys_uart_flush_rx`

```c
blusys_err_t blusys_uart_flush_rx(blusys_uart_t *uart);
```

Discards any data waiting in the RX buffer.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `uart` is NULL, `BLUSYS_ERR_INVALID_STATE` if an RX callback is registered or the handle is closing.

---

### `blusys_uart_set_tx_callback`

```c
blusys_err_t blusys_uart_set_tx_callback(blusys_uart_t *uart,
                                         blusys_uart_tx_callback_t callback,
                                         void *user_ctx);
```

Registers a task-context callback for async TX completion. Cannot be changed while an async TX is pending.

**Returns:** `BLUSYS_ERR_INVALID_STATE` while an async TX is in progress.

---

### `blusys_uart_set_rx_callback`

```c
blusys_err_t blusys_uart_set_rx_callback(blusys_uart_t *uart,
                                         blusys_uart_rx_callback_t callback,
                                         void *user_ctx);
```

Registers a task-context callback for async RX data delivery. While an RX callback is active, `blusys_uart_read()` must not be called.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `uart` is NULL, `BLUSYS_ERR_INVALID_STATE` if the handle is closing.

---

### `blusys_uart_write_async`

```c
blusys_err_t blusys_uart_write_async(blusys_uart_t *uart, const void *data, size_t size);
```

Queues one async TX operation. Returns immediately; the registered TX callback fires on completion.

**Returns:** `BLUSYS_ERR_INVALID_STATE` if another async TX is already pending.

## Lifecycle

1. `blusys_uart_open()` — install driver
2. `blusys_uart_write()` / `blusys_uart_read()` — blocking transfers
3. Optionally register TX/RX callbacks and call `blusys_uart_write_async()`
4. `blusys_uart_close()` — tear down

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
