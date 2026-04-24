# HTTP

HTTP client and server on top of a working IP link.

## At a glance

- client handle allows one in-flight request
- server handlers run on the httpd task
- use WiFi first

## HTTP client

```c
blusys_http_client_t *http;
blusys_http_client_config_t cfg = {
    .url = "http://httpbin.org/get",
    .timeout_ms = 10000,
};

blusys_http_client_open(&cfg, &http);
blusys_http_response_t resp;
blusys_http_client_get(http, NULL, &resp);
free(resp.body);
blusys_http_client_close(http);
```

## HTTP server

```c
static blusys_err_t handle_root(blusys_http_server_req_t *req, void *ctx)
{
    (void)ctx;
    return blusys_http_server_resp_send(req, 200, "text/plain", "Hello", -1);
}
```

## Common mistakes

- forgetting to free `response.body`
- checking `err` but not `status_code`
- calling HTTP before WiFi has an IP
- responding zero or multiple times in a server handler

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- one request at a time per client handle
- handlers should stay short and non-blocking

## Limitations

- route matching is exact
- server routes are fixed at `open()` time

## Example apps

- `examples/reference/connectivity/` (`ref_connectivity_http`)
- `examples/validation/network_services/` (`net_http_server`)
