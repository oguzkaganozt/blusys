# Local Control

Browser-friendly local device control over WiFi with a built-in HTML page and small JSON endpoints.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Local Control Basic](../guides/local-ctrl-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_local_ctrl_status_cb_t`

```c
typedef blusys_err_t (*blusys_local_ctrl_status_cb_t)(char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      void *user_ctx);
```

Optional callback used by `GET /api/status`. Write a JSON document into `json_buf`, set `*out_len`, and return `BLUSYS_OK`.

### `blusys_local_ctrl_action_cb_t`

```c
typedef blusys_err_t (*blusys_local_ctrl_action_cb_t)(const char *action_name,
                                                      const uint8_t *body,
                                                      size_t body_len,
                                                      char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      int *out_status_code,
                                                      void *user_ctx);
```

Action callback used by `POST /api/actions/<name>`. The callback receives the request body, writes a JSON response, and can override the HTTP status code.

### `blusys_local_ctrl_action_t`

```c
typedef struct {
    const char *name;
    const char *label;
    blusys_local_ctrl_action_cb_t handler;
} blusys_local_ctrl_action_t;
```

- `name` must be URL-safe ASCII and becomes part of `/api/actions/<name>`
- `label` is shown on the built-in HTML page; `NULL` falls back to `name`

### `blusys_local_ctrl_config_t`

```c
typedef struct {
    const char *device_name;
    uint16_t http_port;
    const blusys_local_ctrl_action_t *actions;
    size_t action_count;
    size_t max_body_len;
    size_t max_response_len;
    blusys_local_ctrl_status_cb_t status_cb;
    void *user_ctx;
} blusys_local_ctrl_config_t;
```

## Functions

### `blusys_local_ctrl_open`

```c
blusys_err_t blusys_local_ctrl_open(const blusys_local_ctrl_config_t *config,
                                    blusys_local_ctrl_t **out_ctrl);
```

Starts the local control HTTP server and registers the built-in routes plus all action routes.

Built-in routes:
- `GET /` — built-in HTML control page
- `GET /api/info` — module metadata and registered actions
- `GET /api/status` — only when `status_cb` is configured
- `POST /api/actions/<name>` — one exact route per action entry

**Returns:**
- `BLUSYS_OK` — started successfully
- `BLUSYS_ERR_INVALID_ARG` — missing config/device name, invalid action definition, or duplicate action name
- `BLUSYS_ERR_NO_MEM` — allocation failure
- `BLUSYS_ERR_NOT_SUPPORTED` — WiFi not supported on the current target

---

### `blusys_local_ctrl_close`

```c
blusys_err_t blusys_local_ctrl_close(blusys_local_ctrl_t *ctrl);
```

Stops the HTTP server and frees the handle.

---

### `blusys_local_ctrl_is_running`

```c
bool blusys_local_ctrl_is_running(blusys_local_ctrl_t *ctrl);
```

Returns `true` when the underlying HTTP server is still running.

## Lifecycle

```text
WiFi connected
    └── blusys_local_ctrl_open()
            ├── GET /
            ├── GET /api/info
            ├── GET /api/status        (optional)
            └── POST /api/actions/*
        blusys_local_ctrl_close()
```

`local_ctrl` does not connect WiFi for you. Establish WiFi first, then open the module.

## Thread Safety

- `open()` and `close()` must not race with each other on the same handle
- `status_cb` and action callbacks run in the HTTP server task context
- keep callbacks short and non-blocking
- callbacks are informational/request handlers, not lifecycle hooks; do not call `blusys_local_ctrl_close()` from them

## Limitations

- WiFi must already be connected before calling `open()`
- request body size is capped by `max_body_len`
- status and action callbacks are expected to return JSON payloads
- route matching is exact; wildcard action paths are not supported
- mDNS is not managed internally in this first cut; pair it with `blusys_mdns_*` manually if you want `<hostname>.local`

## Example App

See `examples/local_ctrl_basic/`.
