# Connect to a WebSocket Server

Open a full-duplex WebSocket connection, send messages, and receive replies in a callback.

## Problem Statement

You want to establish a persistent, full-duplex connection from an ESP32 to a WebSocket server so that either side can send messages at any time without polling.

## Prerequisites

- A WiFi-capable board: ESP32, ESP32-C3, or ESP32-S3
- A reachable WebSocket server (the default example uses `ws://echo.websocket.events`, a public echo server)
- WiFi credentials

## Minimal Example

```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys_services.h"

static void on_message(blusys_ws_msg_type_t type,
                        const uint8_t *data, size_t len, void *user_ctx)
{
    printf("Received [%s]: %.*s\n",
           type == BLUSYS_WS_MSG_TEXT ? "text" : "binary",
           (int)len, (const char *)data);
}

void app_main(void)
{
    /* 1. Connect to WiFi */
    blusys_wifi_t *wifi;
    blusys_wifi_sta_config_t wifi_cfg = { .ssid = "myssid", .password = "mypassword" };
    blusys_wifi_open(&wifi_cfg, &wifi);
    blusys_wifi_connect(wifi, 10000);

    /* 2. Open WebSocket client */
    blusys_ws_client_t *ws;
    blusys_ws_client_config_t ws_cfg = {
        .url        = "ws://echo.websocket.events",
        .timeout_ms = 10000,
        .message_cb = on_message,
    };
    blusys_ws_client_open(&ws_cfg, &ws);

    /* 3. Connect */
    blusys_ws_client_connect(ws);

    /* 4. Send a text message */
    blusys_ws_client_send_text(ws, "hello from blusys");

    /* 5. Wait for echo, then clean up */
    vTaskDelay(pdMS_TO_TICKS(3000));
    blusys_ws_client_disconnect(ws);
    blusys_ws_client_close(ws);
    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);
}
```

## APIs Used

- `blusys_ws_client_open()` — allocate client with server URL and message callback
- `blusys_ws_client_connect()` — blocking connect; completes the WebSocket handshake
- `blusys_ws_client_send_text()` — send a UTF-8 text frame
- `blusys_ws_client_send()` — send a binary frame
- `blusys_ws_client_disconnect()` / `blusys_ws_client_close()` — graceful shutdown

## Receiving Messages

The `message_cb` is called from the internal receive task each time a data frame arrives:

```c
static void on_message(blusys_ws_msg_type_t type,
                        const uint8_t *data, size_t len, void *user_ctx)
{
    if (type == BLUSYS_WS_MSG_TEXT) {
        /* data is raw bytes — NOT null-terminated; use len */
        printf("text: %.*s\n", (int)len, (const char *)data);
    } else {
        printf("binary: %zu bytes\n", len);
    }
}
```

Keep the callback short. Do not call `connect()`, `disconnect()`, or `close()` from inside it — use a queue or flag to defer to `app_main`.

## Detecting Disconnects

Use the optional `disconnect_cb` to react when the server closes the connection:

```c
static void on_disconnect(void *user_ctx)
{
    printf("Server closed the connection.\n");
    /* Set a flag; do not call ws_client_close() here */
}

blusys_ws_client_config_t cfg = {
    .url           = "ws://myserver.local/ws",
    .timeout_ms    = 10000,
    .message_cb    = on_message,
    .disconnect_cb = on_disconnect,
};
```

## Sending Binary Data

`blusys_ws_client_send()` sends a BINARY frame:

```c
uint8_t buf[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
blusys_ws_client_send(ws, buf, sizeof(buf));
```

## Using TLS (wss://)

For encrypted connections, use a `wss://` URL and optionally provide a PEM CA certificate:

```c
extern const char server_cert[] asm("_binary_server_cert_pem_start");

blusys_ws_client_config_t cfg = {
    .url            = "wss://secure.myserver.com/ws",
    .timeout_ms     = 15000,
    .message_cb     = on_message,
    .server_cert_pem = server_cert,  /* NULL skips CN verification */
};
```

If `server_cert_pem` is `NULL`, certificate CN verification is skipped. Provide a cert for production deployments.

## Common Mistakes

**Not waiting for WiFi before calling `connect()`**

```c
/* Wrong — connect() will fail without an IP address */
blusys_ws_client_connect(ws);

/* Correct — connect WiFi first */
blusys_wifi_connect(wifi, 10000);
blusys_ws_client_connect(ws);
```

**Treating data as a C string in the callback**

`data` in the callback is not null-terminated. Copy it if you need a string:

```c
char buf[128];
size_t n = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
memcpy(buf, data, n);
buf[n] = '\0';
```

**Calling `close()` from inside a callback**

Both `message_cb` and `disconnect_cb` run on the receive task. Calling `close()` or `disconnect()` from within them will deadlock. Post to a FreeRTOS queue or set a flag instead, then act from `app_main`.

## Expected Runtime Behavior

`blusys_ws_client_connect()` performs a TCP connection followed by the HTTP upgrade handshake. Against `echo.websocket.events` on a good WiFi connection this takes roughly 200–600 ms.

After calling `send_text()`, the echo server returns the same payload and `message_cb` fires within a few hundred milliseconds.

## Example App

See `examples/ws_client_basic/`.

## API Reference

For full type definitions and function signatures, see [WebSocket Client API Reference](../modules/ws_client.md).
