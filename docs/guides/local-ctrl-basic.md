# Local Control Basic

Expose a simple browser UI and a few JSON action endpoints on the local network.

## Prerequisites

- supported target: ESP32, ESP32-C3, or ESP32-S3
- WiFi credentials for a 2.4 GHz access point
- an application state structure you want to inspect or control

## Minimal Example

```c
typedef struct {
    bool output_enabled;
    int action_count;
} app_state_t;

static blusys_err_t build_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{
    app_state_t *state = user_ctx;
    int written = snprintf(json_buf,
                           buf_len,
                           "{\"output_enabled\":%s,\"action_count\":%d}",
                           state->output_enabled ? "true" : "false",
                           state->action_count);
    if ((written < 0) || ((size_t)written >= buf_len)) {
        return BLUSYS_ERR_NO_MEM;
    }

    *out_len = (size_t)written;
    return BLUSYS_OK;
}

static blusys_err_t handle_action(const char *action_name,
                                  const uint8_t *body,
                                  size_t body_len,
                                  char *json_buf,
                                  size_t buf_len,
                                  size_t *out_len,
                                  int *out_status_code,
                                  void *user_ctx)
{
    (void) body;
    (void) body_len;

    app_state_t *state = user_ctx;
    if (strcmp(action_name, "toggle") == 0) {
        state->output_enabled = !state->output_enabled;
        state->action_count++;
    }

    int written = snprintf(json_buf,
                           buf_len,
                           "{\"ok\":true,\"output_enabled\":%s}",
                           state->output_enabled ? "true" : "false");
    if ((written < 0) || ((size_t)written >= buf_len)) {
        return BLUSYS_ERR_NO_MEM;
    }

    *out_status_code = 200;
    *out_len = (size_t)written;
    return BLUSYS_OK;
}

static const blusys_local_ctrl_action_t actions[] = {
    { .name = "toggle", .label = "Toggle Output", .handler = handle_action },
};

blusys_local_ctrl_config_t ctrl_cfg = {
    .device_name = "demo-node",
    .http_port = 80,
    .actions = actions,
    .action_count = 1,
    .status_cb = build_status,
    .user_ctx = &state,
};

blusys_local_ctrl_t *ctrl = NULL;
blusys_local_ctrl_open(&ctrl_cfg, &ctrl);
```

## APIs Used

- `blusys_wifi_open()`
- `blusys_wifi_connect()`
- `blusys_local_ctrl_open()`
- `blusys_local_ctrl_is_running()`

## Expected Behavior

- browsing to `http://<device-ip>/` shows the built-in HTML control page
- `GET /api/info` returns a JSON description of the module and registered actions
- `GET /api/status` returns your application state when `status_cb` is configured
- `POST /api/actions/<name>` invokes the matching action callback and returns its JSON response

## Common Mistakes

- opening `local_ctrl` before WiFi is connected
- using action names with spaces or slashes instead of URL-safe names like `toggle_output`
- doing blocking work in action or status callbacks
- returning a response bigger than `max_response_len`
- assuming `local_ctrl` also sets up mDNS; pair it with `blusys_mdns_*` manually if needed

## Example App

See `examples/local_ctrl_basic/`.
