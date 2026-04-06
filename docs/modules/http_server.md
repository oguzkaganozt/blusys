# HTTP Server

Embedded HTTP server for handling incoming requests over WiFi.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [HTTP Server Basics](../guides/http-server-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_http_server_handler_t`

```c
typedef blusys_err_t (*blusys_http_server_handler_t)(blusys_http_server_req_t *req,
                                                      void                     *user_ctx);
```

Callback invoked from the httpd task when a matching request arrives. The handler must call `blusys_http_server_resp_send()` exactly once and return `BLUSYS_OK`. Returning any other value causes httpd to send a 500 response automatically.

### `blusys_http_server_route_t`

```c
typedef struct {
    const char                     *uri;      /* required; exact match, e.g. "/api/data" */
    blusys_http_method_t            method;
    blusys_http_server_handler_t    handler;
    void                           *user_ctx; /* passed through to handler; NULL is fine */
} blusys_http_server_route_t;
```

A single URI/method binding. The routes array must remain valid for the lifetime of the server handle (the pointer is not copied).

`blusys_http_method_t` is defined in `blusys/http_client.h` and includes `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, and `HEAD`.

### `blusys_http_server_config_t`

```c
typedef struct {
    uint16_t                          port;             /* listen port; 0 = default 80 */
    const blusys_http_server_route_t *routes;           /* array of route definitions */
    size_t                            route_count;
    size_t                            max_open_sockets; /* max concurrent connections; 0 = default (4) */
} blusys_http_server_config_t;
```

## Functions

### `blusys_http_server_open`

```c
blusys_err_t blusys_http_server_open(const blusys_http_server_config_t *config,
                                      blusys_http_server_t             **out_handle);
```

Starts the HTTP server and registers all routes.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if config or out_handle is NULL or route_count > 0 with NULL routes, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.

---

### `blusys_http_server_close`

```c
blusys_err_t blusys_http_server_close(blusys_http_server_t *handle);
```

Stops the server and frees the handle. After this call the handle is invalid.

---

### `blusys_http_server_is_running`

```c
bool blusys_http_server_is_running(blusys_http_server_t *handle);
```

Returns `true` if the server is currently running.

---

### `blusys_http_server_req_uri`

```c
const char *blusys_http_server_req_uri(blusys_http_server_req_t *req);
```

Returns the request URI string (null-terminated). Valid only inside a handler callback.

---

### `blusys_http_server_req_content_len`

```c
size_t blusys_http_server_req_content_len(blusys_http_server_req_t *req);
```

Returns the `Content-Length` of the request body (0 if no body).

---

### `blusys_http_server_req_recv`

```c
blusys_err_t blusys_http_server_req_recv(blusys_http_server_req_t *req,
                                          uint8_t                  *buf,
                                          size_t                    buf_len,
                                          size_t                   *out_received);
```

Reads up to `buf_len` bytes of the request body. Sets `*out_received` to the number of bytes actually read. Call repeatedly until `*out_received == 0` or an error occurs.

**Returns:**
- `BLUSYS_OK` — bytes read (check `*out_received`)
- `BLUSYS_ERR_INVALID_ARG` — NULL argument
- `BLUSYS_ERR_TIMEOUT` — receive timeout
- `BLUSYS_ERR_IO` — connection error

---

### `blusys_http_server_req_get_header`

```c
blusys_err_t blusys_http_server_req_get_header(blusys_http_server_req_t *req,
                                                const char               *name,
                                                char                     *out_buf,
                                                size_t                    buf_len);
```

Reads the value of a request header into `out_buf` (null-terminated). Returns `BLUSYS_ERR_IO` if the header is not present or the value does not fit in `out_buf`.

---

### `blusys_http_server_resp_send`

```c
blusys_err_t blusys_http_server_resp_send(blusys_http_server_req_t *req,
                                           int                       status_code,
                                           const char               *content_type,
                                           const char               *body,
                                           int                       body_len);
```

Sends an HTTP response. Call exactly once per handler invocation.

- `status_code`: e.g. `200`, `404`, `500`. Supported codes: 200, 201, 204, 400, 401, 403, 404, 500. Unknown codes fall back to "200 OK".
- `content_type`: MIME type string, e.g. `"text/plain"` or `"application/json"`; `NULL` omits the header.
- `body`: response body; `NULL` sends an empty body.
- `body_len`: byte count; pass `-1` to use `strlen(body)`.

## Lifecycle

```
WiFi connected
    └── blusys_http_server_open()
            └── [server handles requests via registered handler callbacks]
        blusys_http_server_close()
```

The WiFi connection must be established before calling `open()`. The server runs in the background until `close()` is called.

## Thread Safety

Route handlers may be called concurrently for different connections. The ESP-IDF httpd manages internal concurrency. `open()` and `close()` must not be called concurrently with any other function on the same handle.

## Limitations

- The WiFi connection must be established before calling `open()`.
- URI matching is exact; wildcard and prefix matching are not supported in this first cut.
- The handler must call `resp_send()` exactly once. Calling it zero or more than once leads to undefined behavior.
- HTTPS (TLS) is not supported in this first cut; the server listens on plain HTTP only.
- `max_open_sockets` defaults to 4; raising it increases RAM usage.

## Example App

See `examples/http_server_basic/`.
