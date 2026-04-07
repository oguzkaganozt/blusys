# HTTP Server

Run an embedded HTTP server on an ESP32 that responds to incoming requests over WiFi.

## Problem Statement

You want to expose an HTTP endpoint from an ESP32 so that other devices on the local network can retrieve data or send commands over WiFi.

## Prerequisites

- WiFi station connected (see [WiFi guide](wifi-connect.md))
- blusys installed (`blusys version` to verify)

## Minimal Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys_all.h"

static blusys_err_t handle_root(blusys_http_server_req_t *req, void *user_ctx)
{
    return blusys_http_server_resp_send(req, 200, "text/plain", "Hello from blusys!", -1);
}

static const blusys_http_server_route_t routes[] = {
    { .uri = "/", .method = BLUSYS_HTTP_METHOD_GET, .handler = handle_root },
};

void app_main(void)
{
    /* connect WiFi first … */

    blusys_http_server_config_t cfg = {
        .port        = 80,
        .routes      = routes,
        .route_count = sizeof(routes) / sizeof(routes[0]),
    };
    blusys_http_server_t *server = NULL;
    blusys_err_t err = blusys_http_server_open(&cfg, &server);
    if (err != BLUSYS_OK) {
        printf("open failed: %s\n", blusys_err_string(err));
        return;
    }

    printf("server running\n");
    vTaskDelay(portMAX_DELAY);
}
```

Test it:

```sh
curl http://<device-ip>/
# Hello from blusys!
```

## APIs Used

- `blusys_http_server_open()` — starts the server and registers routes
- `blusys_http_server_close()` — stops the server
- `blusys_http_server_resp_send()` — sends an HTTP response from inside a handler
- `blusys_http_server_req_content_len()` — gets the request body size
- `blusys_http_server_req_recv()` — reads the request body

## Handling POST Requests

Read the request body with `req_recv()` then send a response:

```c
static blusys_err_t handle_echo(blusys_http_server_req_t *req, void *user_ctx)
{
    size_t len = blusys_http_server_req_content_len(req);
    if (len == 0) {
        return blusys_http_server_resp_send(req, 200, "text/plain", "", 0);
    }

    char *buf = malloc(len + 1);
    if (buf == NULL) {
        return blusys_http_server_resp_send(req, 500, "text/plain", "Out of memory", -1);
    }

    size_t total = 0;
    while (total < len) {
        size_t chunk = 0;
        blusys_err_t err = blusys_http_server_req_recv(req, (uint8_t *)buf + total,
                                                        len - total, &chunk);
        if (err != BLUSYS_OK) { free(buf); return err; }
        if (chunk == 0) break;
        total += chunk;
    }
    buf[total] = '\0';

    blusys_err_t err = blusys_http_server_resp_send(req, 200, "text/plain", buf, (int)total);
    free(buf);
    return err;
}
```

Register it alongside your other routes:

```c
static const blusys_http_server_route_t routes[] = {
    { .uri = "/",     .method = BLUSYS_HTTP_METHOD_GET,  .handler = handle_root },
    { .uri = "/echo", .method = BLUSYS_HTTP_METHOD_POST, .handler = handle_echo },
};
```

Test it:

```sh
curl -X POST http://<device-ip>/echo -d "hello"
# hello
```

## Reading Request Headers

```c
char auth[64];
blusys_err_t err = blusys_http_server_req_get_header(req, "Authorization", auth, sizeof(auth));
if (err == BLUSYS_OK) {
    printf("Authorization: %s\n", auth);
}
```

## Supported Status Codes

| Code | Meaning |
|------|---------|
| 200 | OK |
| 201 | Created |
| 204 | No Content |
| 400 | Bad Request |
| 401 | Unauthorized |
| 403 | Forbidden |
| 404 | Not Found |
| 500 | Internal Server Error |

## Finding the Device IP

After connecting WiFi, read the IP with `blusys_wifi_get_ip_info()`:

```c
blusys_wifi_ip_info_t ip_info;
blusys_wifi_get_ip_info(wifi, &ip_info);
printf("http://%s/\n", ip_info.ip);
```

## Common Mistakes

**Not sending a response**: the handler must call `resp_send()` exactly once. Forgetting it hangs the connection.

**Calling `resp_send()` twice**: only the first send takes effect; the second returns an error from the underlying httpd stack.

**NULL handler in a route**: every route must have a non-NULL `handler`. A NULL handler causes undefined behavior when the URI is matched.

**Starting the server before WiFi is connected**: `open()` will succeed but clients will get connection refused since the network stack is not ready.

**Body larger than available heap**: always check `content_len` before allocating, and add a cap if the endpoint is exposed to untrusted clients.

## Example App

See `examples/http_server_basic/`.

## API Reference

For full type definitions and function signatures, see [HTTP Server API Reference](../modules/http_server.md).
