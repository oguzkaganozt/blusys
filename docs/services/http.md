# HTTP

HTTP client and server services for communicating over WiFi.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

---

## HTTP Client

Blocking HTTP/HTTPS client for fetching and posting data over a WiFi connection.

### Quick Example

```c
#include <stdlib.h>
#include "blusys/blusys.h"

blusys_http_client_t *http;
blusys_http_client_config_t http_cfg = {
    .url        = "http://httpbin.org/get",
    .timeout_ms = 10000,
};
blusys_http_client_open(&http_cfg, &http);

blusys_http_response_t resp;
blusys_http_client_get(http, NULL, &resp);

printf("Status: %d\nBody: %s\n", resp.status_code, (char *)resp.body);
free(resp.body);

blusys_http_client_close(http);
```

### Client Types

#### `blusys_http_method_t`

```c
typedef enum {
    BLUSYS_HTTP_METHOD_GET = 0,
    BLUSYS_HTTP_METHOD_POST,
    BLUSYS_HTTP_METHOD_PUT,
    BLUSYS_HTTP_METHOD_PATCH,
    BLUSYS_HTTP_METHOD_DELETE,
    BLUSYS_HTTP_METHOD_HEAD,
} blusys_http_method_t;
```

#### `blusys_http_header_t`

```c
typedef struct {
    const char *name;
    const char *value;
} blusys_http_header_t;
```

#### `blusys_http_client_config_t`

```c
typedef struct {
    const char *url;
    const char *cert_pem;
    int         timeout_ms;
    size_t      max_response_body_size;
} blusys_http_client_config_t;
```

#### `blusys_http_response_t`

```c
typedef struct {
    int    status_code;
    void  *body;           /* heap-allocated, null-terminated; caller must free() */
    size_t body_size;
    bool   body_truncated;
} blusys_http_response_t;
```

### Client Functions

- `blusys_http_client_open(config, &client)` — allocate and initialize
- `blusys_http_client_close(client)` — close connection and free handle
- `blusys_http_client_get(client, path, &resp)` — blocking GET
- `blusys_http_client_post(client, path, content_type, body, body_size, &resp)` — blocking POST
- `blusys_http_client_request(client, &request, &resp)` — full request with custom method/headers

### Client Lifecycle

Open -> request (repeat) -> close. The handle reuses TCP connections via HTTP/1.1 keep-alive.

### Client Thread Safety

One request at a time per handle. Concurrent calls return `BLUSYS_ERR_INVALID_STATE`.

---

## HTTP Server

Embedded HTTP server for handling incoming requests over WiFi.

### Server Quick Example

```c
#include "blusys/blusys.h"

static blusys_err_t handle_root(blusys_http_server_req_t *req, void *ctx) {
    return blusys_http_server_resp_send(req, 200, "text/plain", "Hello", -1);
}

blusys_http_server_route_t routes[] = {
    { .uri = "/", .method = BLUSYS_HTTP_METHOD_GET, .handler = handle_root },
};
blusys_http_server_config_t cfg = { .routes = routes, .route_count = 1 };
blusys_http_server_t *server;
blusys_http_server_open(&cfg, &server);
```

### Server Types

#### `blusys_http_server_handler_t`

```c
typedef blusys_err_t (*blusys_http_server_handler_t)(blusys_http_server_req_t *req, void *user_ctx);
```

#### `blusys_http_server_route_t`

```c
typedef struct {
    const char                     *uri;
    blusys_http_method_t            method;
    blusys_http_server_handler_t    handler;
    void                           *user_ctx;
} blusys_http_server_route_t;
```

#### `blusys_http_server_config_t`

```c
typedef struct {
    uint16_t                          port;
    const blusys_http_server_route_t *routes;
    size_t                            route_count;
    size_t                            max_open_sockets;
} blusys_http_server_config_t;
```

### Server Functions

- `blusys_http_server_open(config, &handle)` — start server and register routes
- `blusys_http_server_close(handle)` — stop server
- `blusys_http_server_is_running(handle)` — check if running
- `blusys_http_server_req_uri(req)` — get request URI
- `blusys_http_server_req_content_len(req)` — get body length
- `blusys_http_server_req_recv(req, buf, len, &received)` — read body
- `blusys_http_server_req_get_header(req, name, buf, len)` — read header
- `blusys_http_server_resp_send(req, status, content_type, body, len)` — send response

### Common Mistakes

- Forgetting to `free(response.body)` after client requests
- Checking `err` but not `status_code` — HTTP 4xx/5xx returns `BLUSYS_OK`
- Calling HTTP functions before WiFi is connected
- Calling `resp_send()` zero or more than once in a server handler

## Example Apps

- `examples/reference/connectivity/` (`ref_connectivity_http` scenario) — GET request with WiFi
- `examples/validation/network_services/` (`net_http_server` scenario) — server with route handlers
