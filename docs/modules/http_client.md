# HTTP Client

Blocking HTTP/HTTPS client for fetching and posting data over a WiFi connection.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_http_method_t`

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

### `blusys_http_header_t`

```c
typedef struct {
    const char *name;
    const char *value;
} blusys_http_header_t;
```

Array of name/value pairs passed to `blusys_http_client_request()`. Terminate the array with `{ NULL, NULL }`.

### `blusys_http_client_config_t`

```c
typedef struct {
    const char *url;                     /* required; copied internally at open time */
    const char *cert_pem;                /* PEM CA cert for HTTPS; NULL skips verification */
    int         timeout_ms;              /* default timeout; BLUSYS_TIMEOUT_FOREVER to block */
    size_t      max_response_body_size;  /* hard cap on buffered body bytes; 0 = unlimited */
} blusys_http_client_config_t;
```

`cert_pem` must remain valid for the lifetime of the handle (the pointer is not copied).

### `blusys_http_request_t`

```c
typedef struct {
    blusys_http_method_t        method;
    const char                 *path;       /* optional path/query override; NULL = use config URL */
    const blusys_http_header_t *headers;    /* NULL-terminated array, or NULL */
    const void                 *body;
    size_t                      body_size;
    int                         timeout_ms; /* per-request override; 0 = use handle default */
} blusys_http_request_t;
```

### `blusys_http_response_t`

```c
typedef struct {
    int    status_code;
    void  *body;           /* heap-allocated, null-terminated; caller must free() */
    size_t body_size;      /* bytes written, excluding null terminator */
    bool   body_truncated; /* true if max_response_body_size was reached */
} blusys_http_response_t;
```

`body` is always null-terminated for text convenience. The caller must call `free(response.body)` after use.

## Functions

### `blusys_http_client_open`

```c
blusys_err_t blusys_http_client_open(const blusys_http_client_config_t *config,
                                     blusys_http_client_t **out_client);
```

Allocates and initialises an HTTP client for the given base URL. The underlying connection is not opened until the first request.

**Parameters:**
- `config` — URL, optional TLS certificate, timeout, and body size cap (required)
- `out_client` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if config or url is NULL, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.

---

### `blusys_http_client_close`

```c
blusys_err_t blusys_http_client_close(blusys_http_client_t *client);
```

Closes any open connection and frees the handle. After this call the handle is invalid.

---

### `blusys_http_client_request`

```c
blusys_err_t blusys_http_client_request(blusys_http_client_t        *client,
                                        const blusys_http_request_t *request,
                                        blusys_http_response_t      *out_response);
```

Performs a synchronous HTTP request. Blocks until the response is fully received, a network error occurs, or the timeout expires.

On `BLUSYS_OK`, `out_response->body` is heap-allocated and must be freed by the caller with `free()`.

HTTP 4xx/5xx status codes are **not** translated to error return values — check `out_response->status_code` to distinguish application-level failures from network errors.

**Returns:**
- `BLUSYS_OK` — request completed; check `status_code` for application errors
- `BLUSYS_ERR_INVALID_ARG` — NULL pointer or invalid timeout
- `BLUSYS_ERR_INVALID_STATE` — another request is already in progress on this handle
- `BLUSYS_ERR_TIMEOUT` — request did not complete within the timeout
- `BLUSYS_ERR_NO_MEM` — response body allocation failed
- `BLUSYS_ERR_IO` — network-level failure (DNS, TCP, TLS handshake)

---

### `blusys_http_client_get`

```c
blusys_err_t blusys_http_client_get(blusys_http_client_t   *client,
                                    const char             *path,
                                    blusys_http_response_t *out_response);
```

Convenience wrapper for GET requests using the handle's default timeout. Pass `NULL` for `path` to use the URL set at open time.

---

### `blusys_http_client_post`

```c
blusys_err_t blusys_http_client_post(blusys_http_client_t   *client,
                                     const char             *path,
                                     const char             *content_type,
                                     const void             *body,
                                     size_t                  body_size,
                                     blusys_http_response_t *out_response);
```

Convenience wrapper for POST requests. Pass `NULL` for `content_type` to omit the `Content-Type` header. Pass `NULL` for `path` to use the URL set at open time.

## Lifecycle

```
blusys_http_client_open()
    └── blusys_http_client_get() / blusys_http_client_post() / blusys_http_client_request()
            [repeat as needed]
blusys_http_client_close()
```

The handle may be reused for multiple requests. ESP-IDF reuses the underlying TCP/TLS connection when the server supports persistent connections (HTTP/1.1 keep-alive).

## Thread Safety

`blusys_http_client_request()` (and the convenience wrappers) serialise concurrent callers: the second concurrent call returns `BLUSYS_ERR_INVALID_STATE` immediately rather than queuing behind the first. `open()` and `close()` must not be called concurrently with `request()`.

## Limitations

- One request at a time per handle.
- No streaming API — the full response body is buffered in heap memory before returning.
- HTTPS certificate verification requires a valid PEM cert in `config.cert_pem`; passing `NULL` skips verification.
- The WiFi connection must be established before calling `open()` or any request function.
