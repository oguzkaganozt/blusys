# Make an HTTP GET Request

## Problem Statement

You want to fetch data from a REST API or web server over WiFi and process the response body.

## Prerequisites

- a supported board with WiFi (ESP32, ESP32-C3, or ESP32-S3)
- the SSID and password of a 2.4 GHz access point
- an HTTP server reachable from the board's network

## Minimal Example

```c
#include <stdlib.h>
#include "blusys/blusys.h"

/* 1. Connect to WiFi first */
blusys_wifi_t *wifi;
blusys_wifi_sta_config_t wifi_cfg = { .ssid = "MyNetwork", .password = "MyPassword" };
blusys_wifi_open(&wifi_cfg, &wifi);
blusys_wifi_connect(wifi, 10000);

/* 2. Open an HTTP client */
blusys_http_client_t *http;
blusys_http_client_config_t http_cfg = {
    .url        = "http://httpbin.org/get",
    .timeout_ms = 10000,
};
blusys_http_client_open(&http_cfg, &http);

/* 3. Perform a GET request */
blusys_http_response_t resp;
blusys_http_client_get(http, NULL, &resp);

printf("Status: %d\n", resp.status_code);
printf("Body: %s\n", (char *)resp.body);

/* 4. Free the response body */
free(resp.body);

/* 5. Clean up */
blusys_http_client_close(http);
blusys_wifi_disconnect(wifi);
blusys_wifi_close(wifi);
```

## APIs Used

- `blusys_http_client_open()` — allocates the client and configures the base URL and timeout
- `blusys_http_client_get()` — performs a blocking GET and returns the response
- `blusys_http_client_close()` — releases the client
- `blusys_wifi_*` — WiFi must be connected before any HTTP request

## POST Request

Use `blusys_http_client_post()` to send data:

```c
const char *json = "{\"sensor\": \"temperature\", \"value\": 25}";
blusys_http_response_t resp;

blusys_http_client_post(http,
                        "/api/data",
                        "application/json",
                        json, strlen(json),
                        &resp);

printf("Status: %d\n", resp.status_code);
free(resp.body);
```

## Custom Headers

Use `blusys_http_client_request()` directly when you need custom headers:

```c
blusys_http_header_t headers[] = {
    { "Authorization", "Bearer mytoken" },
    { "Accept",        "application/json" },
    { NULL, NULL },
};

blusys_http_request_t req = {
    .method     = BLUSYS_HTTP_METHOD_GET,
    .path       = "/api/protected",
    .headers    = headers,
    .timeout_ms = 5000,
};

blusys_http_response_t resp;
blusys_http_client_request(http, &req, &resp);

printf("Status: %d  Body: %s\n", resp.status_code, (char *)resp.body);
free(resp.body);
```

## HTTP Status Codes

A successful return from `blusys_http_client_get()` (i.e. `BLUSYS_OK`) means the network round-trip completed. It does not mean the server returned 200. Always check `response.status_code`:

```c
blusys_err_t err = blusys_http_client_get(http, NULL, &resp);
if (err != BLUSYS_OK) {
    printf("Network error: %s\n", blusys_err_string(err));
} else if (resp.status_code != 200) {
    printf("Server error: %d\n", resp.status_code);
} else {
    printf("OK: %s\n", (char *)resp.body);
}
free(resp.body);
```

## Body Size Cap

To prevent large responses from exhausting heap memory, set `max_response_body_size` in the config:

```c
blusys_http_client_config_t cfg = {
    .url                    = "http://example.com/large",
    .timeout_ms             = 10000,
    .max_response_body_size = 4096,   /* truncate after 4 KB */
};
```

Check `response.body_truncated` to detect truncation.

## Expected Runtime Behavior

- `open()` allocates the client; no network activity until the first request
- `get()` / `post()` / `request()` blocks until the response body is fully received
- the ESP-IDF driver reuses the TCP connection across requests to the same host (keep-alive)
- if the server closes the connection, the driver reconnects automatically on the next request

## Common Mistakes

- forgetting to call `free(response.body)` — the body buffer is heap-allocated and owned by the caller
- checking `err` but not `status_code` — a 404 or 500 returns `BLUSYS_OK` with a non-200 status code
- calling `http_client_open()` before WiFi is connected — the first request will fail with `BLUSYS_ERR_IO`
- passing a relative path to `open()` instead of a full URL — `url` must include scheme and host

## Example App

See `examples/http_client_basic/` for a runnable example that connects to WiFi and performs a GET request.


## API Reference

For full type definitions and function signatures, see [HTTP Client API Reference](../modules/http_client.md).
