# Local Control

Browser-friendly local device control over WiFi with a built-in HTML page and small JSON endpoints.

## Quick Example

```c
#include "blusys/blusys.h"

static blusys_err_t on_status(char *buf, size_t buf_len, size_t *out_len, void *ctx)
{
    (void)ctx;
    int n = snprintf(buf, buf_len, "{\"ok\":true}");
    *out_len = (size_t)n;
    return BLUSYS_OK;
}

static blusys_err_t on_restart(const char *name, const uint8_t *body, size_t body_len,
                               char *buf, size_t buf_len, size_t *out_len,
                               int *out_status, void *ctx)
{
    (void)name; (void)body; (void)body_len; (void)ctx;
    int n = snprintf(buf, buf_len, "{\"restarting\":true}");
    *out_len    = (size_t)n;
    *out_status = 200;
    /* schedule the restart from another task */
    return BLUSYS_OK;
}

void app_main(void)
{
    /* WiFi must already be connected. */

    blusys_local_ctrl_action_t actions[] = {
        { .name = "restart", .label = "Restart", .handler = on_restart },
    };

    blusys_local_ctrl_t *ctrl;
    blusys_local_ctrl_config_t cfg = {
        .device_name  = "My Device",
        .actions      = actions,
        .action_count = 1,
        .status_cb    = on_status,
    };
    blusys_local_ctrl_open(&cfg, &ctrl);
}
```

Built-in routes:

- `GET /` — built-in HTML control page
- `GET /api/info` — module metadata and registered actions
- `GET /api/status` — only when `status_cb` is configured
- `POST /api/actions/<name>` — one exact route per action entry

## Common Mistakes

- **opening before WiFi has an IP** — the server binds on `open()`; establish WiFi first
- **duplicate action names** — `open()` returns `BLUSYS_ERR_INVALID_ARG`
- **calling `close()` from a handler** — the handler runs in the HTTP server task; schedule the close elsewhere
- **wildcard action paths** — only exact route matches are supported

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- `open()` and `close()` must not race with each other on the same handle
- `status_cb` and action callbacks run in the HTTP server task context — keep them short and non-blocking
- callbacks are request handlers, not lifecycle hooks; do not call `close()` from them

## ISR Notes

No ISR-safe calls are defined for the local-control module.

## Limitations

- WiFi must already be connected before calling `open()`
- request body size is capped by `max_body_len`
- status and action callbacks are expected to return JSON payloads
- route matching is exact; wildcard action paths are not supported
- mDNS is not managed internally; pair it with `blusys_mdns_*` manually for `<hostname>.local` access

## Example App

See `examples/validation/network_services/` (`net_local_ctrl` scenario).
