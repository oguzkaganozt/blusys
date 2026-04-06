# TWAI

Classic CAN-style communication using the ESP32's TWAI controller. Supports sending and receiving standard 11-bit frames with an async RX callback.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [TWAI Basics](../guides/twai-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_twai_t`

```c
typedef struct blusys_twai blusys_twai_t;
```

Opaque handle returned by `blusys_twai_open()`.

### `blusys_twai_frame_t`

```c
typedef struct {
    uint32_t id;         /* 11-bit standard CAN identifier */
    bool remote_frame;   /* true for remote frames (RTR bit) */
    uint8_t dlc;         /* data length code (0–8) */
    uint8_t data[8];     /* payload bytes */
    size_t data_len;     /* number of valid bytes in data; 0 for remote frames */
} blusys_twai_frame_t;
```

### `blusys_twai_rx_callback_t`

```c
typedef bool (*blusys_twai_rx_callback_t)(blusys_twai_t *twai,
                                          const blusys_twai_frame_t *frame,
                                          void *user_ctx);
```

Called in ISR context when a frame is received. Return `true` to request a FreeRTOS yield.

## Functions

### `blusys_twai_open`

```c
blusys_err_t blusys_twai_open(int tx_pin,
                              int rx_pin,
                              uint32_t bitrate,
                              blusys_twai_t **out_twai);
```

Installs the TWAI driver. Does not join the bus — call `blusys_twai_start()` to begin.

**Parameters:**
- `tx_pin` — GPIO for TWAI TX (connects to transceiver TXD)
- `rx_pin` — GPIO for TWAI RX (connects to transceiver RXD)
- `bitrate` — bus bitrate in bits/s (e.g. 125000, 500000, 1000000)
- `out_twai` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.

---

### `blusys_twai_close`

```c
blusys_err_t blusys_twai_close(blusys_twai_t *twai);
```

Uninstalls the driver and frees the handle. Call `blusys_twai_stop()` first.

---

### `blusys_twai_start`

```c
blusys_err_t blusys_twai_start(blusys_twai_t *twai);
```

Joins the CAN bus. After this call, received frames trigger the RX callback and writes are allowed.

---

### `blusys_twai_stop`

```c
blusys_err_t blusys_twai_stop(blusys_twai_t *twai);
```

Leaves the bus. No frames are transmitted or received after this call.

---

### `blusys_twai_write`

```c
blusys_err_t blusys_twai_write(blusys_twai_t *twai,
                               const blusys_twai_frame_t *frame,
                               int timeout_ms);
```

Transmits one frame. Blocks until the frame is sent or the timeout expires.

**Parameters:**
- `twai` — handle
- `frame` — frame to send; `id` must be a valid 11-bit value, `dlc` must match `data_len`
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if called before `blusys_twai_start()`, `BLUSYS_ERR_TIMEOUT`, `BLUSYS_ERR_IO` if the backend reports transmit failure, `BLUSYS_ERR_INVALID_ARG` for invalid frame fields.

---

### `blusys_twai_set_rx_callback`

```c
blusys_err_t blusys_twai_set_rx_callback(blusys_twai_t *twai,
                                         blusys_twai_rx_callback_t callback,
                                         void *user_ctx);
```

Registers the ISR callback for received frames. Pass NULL to clear.

## Lifecycle

1. `blusys_twai_open()` — install driver
2. `blusys_twai_set_rx_callback()` — register RX handler
3. `blusys_twai_start()` — join bus
4. `blusys_twai_write()` — send frames
5. `blusys_twai_stop()` — leave bus
6. `blusys_twai_close()` — uninstall

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_twai_close()` concurrently with other calls on the same handle
- the RX callback runs in ISR context

## Limitations

- external transceiver hardware is required (the TWAI pins connect to transceiver TXD/RXD, not directly to the bus)
- receive uses a callback only; no blocking read API
- only classic standard frames (11-bit ID) are supported; CAN FD is out of scope
- no filter, loopback, listen-only, or bus-off recovery API

## Example App

See `examples/twai_basic/`.
