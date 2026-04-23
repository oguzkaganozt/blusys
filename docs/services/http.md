# HTTP

HTTP **client** and **server** on top of a working IP link (usually [WiFi](wifi.md) associated and addressed).

> **Client:** one in-flight request per handle. **Server:** bind and respond from callbacks; see thread notes under each section.

## HTTP client

Blocking HTTP/HTTPS client for fetch/post over WiFi.

### Quick example

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

Supported methods: `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`. Use `blusys_http_client_request()` for custom methods, headers, or a per-request body.

### Client Thread Safety

One request at a time per handle. Concurrent `request()` calls on the same handle return `BLUSYS_ERR_INVALID_STATE`.

## HTTP Server

Embedded HTTP server for handling incoming requests.

### Quick example

```c
#include "blusys/blusys.h"

static blusys_err_t handle_root(blusys_http_server_req_t *req, void *ctx)
{
    (void)ctx;
    return blusys_http_server_resp_send(req, 200, "text/plain", "Hello", -1);
}

blusys_http_server_route_t routes[] = {
    { .uri = "/", .method = BLUSYS_HTTP_METHOD_GET, .handler = handle_root },
};
blusys_http_server_config_t cfg = { .routes = routes, .route_count = 1 };
blusys_http_server_t *server;
blusys_http_server_open(&cfg, &server);
```

Handlers run on the httpd task. Each handler must send exactly one response via `blusys_http_server_resp_send()` and return `BLUSYS_OK`; any other return value triggers an automatic 500.

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Common mistakes

- forgetting to `free(response.body)` after client requests
- checking `err` but not `status_code` — HTTP 4xx/5xx responses still return `BLUSYS_OK`
- calling HTTP functions before WiFi has an IP address
- calling `resp_send()` zero times, or more than once, in a server handler
- route matching is exact; there is no wildcard or parameter support

## Thread Safety

- **client**: one request at a time per handle; concurrent calls return `BLUSYS_ERR_INVALID_STATE`
- **server**: handlers run on the httpd task; keep them short and non-blocking
- do not call `blusys_http_server_close()` from inside a handler

## ISR Notes

No ISR-safe calls are defined for the HTTP modules.

## Limitations

- the server route list is fixed at `open()` time; dynamic route registration is not supported
- TLS verification is on by default when a `cert_pem` is supplied; NULL disables it (fine for plain HTTP only)
- on constrained targets, large response bodies benefit from capping `max_response_body_size`

## Example Apps

- `examples/reference/connectivity/` (`ref_connectivity_http` scenario) — GET request with WiFi
- `examples/validation/network_services/` (`net_http_server` scenario) — server with route handlers
