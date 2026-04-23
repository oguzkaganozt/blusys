# WebSocket

Client API for full-duplex, real-time communication with a server over WiFi.

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

The `server_cert_pem` and `subprotocol` pointers are not copied; keep them valid for the lifetime of the handle.

## Common Mistakes

- **calling `connect()` before WiFi has an IP** — returns `BLUSYS_ERR_IO` at the TCP stage
- **blocking inside `message_cb` or `disconnect_cb`** — both run on the receive task; keep them short
- **calling `connect()` / `disconnect()` / `close()` from a callback** — this deadlocks
- **expecting fragmentation reassembly** — frames are delivered individually; each `message_cb` call is one WebSocket frame
- **`wss://` without `server_cert_pem`** — CN verification is skipped; pass a PEM CA cert for production

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

Requires WiFi to be connected before calling `blusys_ws_client_connect()`.

## Thread Safety

- `send_text()` and `send()` serialise concurrent callers via an internal lock
- `open()` and `close()` must not be called concurrently with any other function on the same handle
- `is_connected()` is safe from any task

## ISR Notes

No ISR-safe calls are defined for the WebSocket client module.

## Limitations

- fragmented WebSocket messages are delivered as individual callbacks — no reassembly
- the receive buffer is 1024 bytes; larger frames are split across multiple callbacks
- the `message_cb` and `disconnect_cb` must not call `connect()`, `disconnect()`, or `close()` on the same handle
- only one connection per handle at a time

## Example App

See `examples/validation/network_services/` (`net_ws_client` scenario).
