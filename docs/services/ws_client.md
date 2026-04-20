# WebSocket Client

Full-duplex WebSocket client for real-time communication with a server over WiFi.


## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

Requires WiFi to be connected before calling `blusys_ws_client_connect()`.

## Quick Example

```c
#include "blusys/blusys.h"

static void on_message(blusys_ws_msg_type_t type, const uint8_t *data,
                       size_t len, void *ctx)
{
    if (type == BLUSYS_WS_MSG_TEXT) {
        printf("text: %.*s\n", (int)len, (const char *)data);
    }
}

blusys_ws_client_t *ws = NULL;
blusys_ws_client_config_t cfg = {
    .url        = "ws://echo.example.com/",
    .message_cb = on_message,
    .timeout_ms = 10000,
};
blusys_ws_client_open(&cfg, &ws);
blusys_ws_client_connect(ws);

blusys_ws_client_send_text(ws, "hello");

/* ... later ... */
blusys_ws_client_disconnect(ws);
blusys_ws_client_close(ws);
```

## Types

### `blusys_ws_msg_type_t`

```c
typedef enum {
    BLUSYS_WS_MSG_TEXT   = 0,
    BLUSYS_WS_MSG_BINARY = 1,
} blusys_ws_msg_type_t;
```

Identifies whether a received data frame carries UTF-8 text or raw binary bytes.

### `blusys_ws_message_cb_t`

```c
typedef void (*blusys_ws_message_cb_t)(blusys_ws_msg_type_t  type,
                                        const uint8_t        *data,
                                        size_t                len,
                                        void                 *user_ctx);
```

Called from the internal receive task when a data frame arrives. `data` is raw bytes and is **not** null-terminated for text frames; use `len` to determine length. The callback must not block for extended periods and must not call `connect()`, `disconnect()`, or `close()` on the same handle.

### `blusys_ws_disconnect_cb_t`

```c
typedef void (*blusys_ws_disconnect_cb_t)(void *user_ctx);
```

Called from the receive task when the server closes the connection or a network error occurs. Optional; pass `NULL` if not needed.

### `blusys_ws_client_config_t`

```c
typedef struct {
    const char                  *url;            /* required; "ws://" or "wss://" */
    blusys_ws_message_cb_t       message_cb;     /* required; called on incoming data frames */
    blusys_ws_disconnect_cb_t    disconnect_cb;  /* optional; called on disconnect */
    void                        *user_ctx;       /* passed through to callbacks */
    int                          timeout_ms;     /* connect/send timeout; BLUSYS_TIMEOUT_FOREVER to block */
    const char                  *subprotocol;    /* optional Sec-WebSocket-Protocol value */
    const char                  *server_cert_pem; /* PEM CA cert for wss://; NULL skips CN check */
} blusys_ws_client_config_t;
```

`server_cert_pem` and `subprotocol` must remain valid for the lifetime of the handle (the pointers are not copied).

## Functions

### `blusys_ws_client_open`

```c
blusys_err_t blusys_ws_client_open(const blusys_ws_client_config_t *config,
                                    blusys_ws_client_t             **out_handle);
```

Allocates and initialises a WebSocket client. Does not connect.

**Parameters:**
- `config` — URL, callbacks, timeout, and optional TLS cert (required)
- `out_handle` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if config, url, message_cb, or out_handle is NULL or timeout_ms is invalid, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.

---

### `blusys_ws_client_close`

```c
blusys_err_t blusys_ws_client_close(blusys_ws_client_t *handle);
```

Disconnects if still connected, then frees the handle. After this call the handle is invalid.

---

### `blusys_ws_client_connect`

```c
blusys_err_t blusys_ws_client_connect(blusys_ws_client_t *handle);
```

Connects to the server. Blocks until the TCP connection and WebSocket handshake complete, the `timeout_ms` expires, or a network error occurs. On success, starts the internal receive task.

**Returns:**
- `BLUSYS_OK` — handshake complete; receive task is running
- `BLUSYS_ERR_INVALID_ARG` — NULL handle
- `BLUSYS_ERR_INVALID_STATE` — already connected
- `BLUSYS_ERR_IO` — TCP connection, DNS resolution, or WebSocket handshake failure
- `BLUSYS_ERR_NO_MEM` — receive task creation failed

---

### `blusys_ws_client_disconnect`

```c
blusys_err_t blusys_ws_client_disconnect(blusys_ws_client_t *handle);
```

Closes the transport and waits for the receive task to finish.

**Returns:** `BLUSYS_OK` (immediately if not connected).

---

### `blusys_ws_client_send_text`

```c
blusys_err_t blusys_ws_client_send_text(blusys_ws_client_t *handle, const char *text);
```

Sends a null-terminated string as a WebSocket TEXT frame.

**Returns:**
- `BLUSYS_OK` — frame sent
- `BLUSYS_ERR_INVALID_ARG` — NULL handle or text
- `BLUSYS_ERR_INVALID_STATE` — not connected
- `BLUSYS_ERR_IO` — transport send failure

---

### `blusys_ws_client_send`

```c
blusys_err_t blusys_ws_client_send(blusys_ws_client_t *handle,
                                    const uint8_t      *data,
                                    size_t              len);
```

Sends raw bytes as a WebSocket BINARY frame.

**Returns:** same as `blusys_ws_client_send_text`.

---

### `blusys_ws_client_is_connected`

```c
bool blusys_ws_client_is_connected(blusys_ws_client_t *handle);
```

Returns `true` if the client currently has an active connection. Thread-safe.

## Lifecycle

```
blusys_ws_client_open()
    └── blusys_ws_client_connect()           ← starts receive task
            └── blusys_ws_client_send_text()
                blusys_ws_client_send()
                [message_cb fires on incoming frames]
        blusys_ws_client_disconnect()        ← stops receive task
blusys_ws_client_close()
```

The handle may be reconnected after `disconnect()` by calling `connect()` again.

## Thread Safety

`send_text()` and `send()` serialise concurrent callers via an internal lock. `open()` and `close()` must not be called concurrently with any other function on the same handle.

## Limitations

- WiFi must be connected before calling `connect()`.
- Fragmented WebSocket messages are delivered as individual callbacks — no reassembly is performed. Each call to `message_cb` corresponds to one WebSocket frame.
- The receive buffer is 1024 bytes; frames larger than this will be split across multiple callbacks.
- For `wss://` without `server_cert_pem`, certificate CN verification is skipped. Pass a PEM CA cert for production use.
- The `message_cb` and `disconnect_cb` are called from the receive task; they must not call `connect()`, `disconnect()`, or `close()` on the same handle.
- Only one connection per handle at a time.

## Example App

See `examples/validation/network_services/` (`net_ws_client` scenario).
