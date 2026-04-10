# WiFi

Station-mode WiFi for connecting the ESP32 to an existing access point.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Quick Example

```c
blusys_wifi_t *wifi;
blusys_wifi_ip_info_t ip;

blusys_wifi_sta_config_t cfg = {
    .ssid               = "MyNetwork",
    .password           = "MyPassword",
    .auto_reconnect     = true,
    .reconnect_delay_ms = 1000,
    .max_retries        = -1,
};

blusys_wifi_open(&cfg, &wifi);
blusys_wifi_connect(wifi, 10000);   /* block up to 10 s */
blusys_wifi_get_ip_info(wifi, &ip);

printf("IP: %s  GW: %s\n", ip.ip, ip.gateway);

blusys_wifi_disconnect(wifi);
blusys_wifi_close(wifi);
```

## Open vs Connect

`open()` and `connect()` are deliberately separate so you can initialize the stack at boot and defer the connection attempt until needed.

```c
/* init at boot */
blusys_wifi_open(&cfg, &wifi);

/* connect when required */
if (need_network) {
    blusys_wifi_connect(wifi, 10000);
}
```

## Open Networks

Pass `NULL` or `""` as the password for open (unsecured) networks:

```c
blusys_wifi_sta_config_t cfg = {
    .ssid     = "OpenNet",
    .password = NULL,
};
```

## Event Callback

You can subscribe to WiFi lifecycle events without changing the blocking `connect()` flow:

```c
static void on_wifi_event(blusys_wifi_t *wifi, blusys_wifi_event_t event,
                          const blusys_wifi_event_info_t *info, void *user_ctx)
{
    (void) wifi;
    (void) user_ctx;

    if ((event == BLUSYS_WIFI_EVENT_DISCONNECTED) && (info != NULL)) {
        printf("disconnect reason=%d raw=%d\n",
               (int)info->disconnect_reason,
               info->raw_disconnect_reason);
    }
}

blusys_wifi_sta_config_t cfg = {
    .ssid               = "MyNetwork",
    .password           = "MyPassword",
    .auto_reconnect     = true,
    .reconnect_delay_ms = 1000,
    .max_retries        = -1,
    .on_event           = on_wifi_event,
};
```

Keep callbacks short. Hand off long-running work to your own FreeRTOS task or queue. Treat callbacks as informational only: do not call `blusys_wifi_connect()`, `blusys_wifi_disconnect()`, or `blusys_wifi_close()` from them.

If you need the exact driver reason for diagnostics, read `info->raw_disconnect_reason` in the callback or call `blusys_wifi_get_last_disconnect_reason_raw()` after `connect()` fails.

## Common Mistakes

- calling `get_ip_info()` before `connect()` succeeds — returns `BLUSYS_ERR_INVALID_STATE`
- using a 5 GHz SSID — ESP32 radio supports 2.4 GHz only
- forgetting to call `close()` — the NVS partition and TCP/IP stack are not released until `close()`
- doing heavy work or WiFi lifecycle calls inside the callback — keep callbacks short, non-blocking, and informational only

## Types

### `blusys_wifi_sta_config_t`

```c
typedef struct {
    const char *ssid;               /* AP SSID (required) */
    const char *password;           /* AP password, NULL or "" for open networks */
    bool auto_reconnect;            /* reconnect after unexpected disconnect */
    int reconnect_delay_ms;         /* delay before reconnect; <= 0 uses default */
    int max_retries;                /* 0 disables retries, -1 retries forever */
    blusys_wifi_event_cb_t on_event;/* optional async lifecycle callback */
    void *user_ctx;                 /* passed back in on_event */
} blusys_wifi_sta_config_t;
```

### `blusys_wifi_event_t`

```c
typedef enum {
    BLUSYS_WIFI_EVENT_STARTED,
    BLUSYS_WIFI_EVENT_CONNECTING,
    BLUSYS_WIFI_EVENT_CONNECTED,
    BLUSYS_WIFI_EVENT_GOT_IP,
    BLUSYS_WIFI_EVENT_DISCONNECTED,
    BLUSYS_WIFI_EVENT_RECONNECTING,
    BLUSYS_WIFI_EVENT_STOPPED,
} blusys_wifi_event_t;
```

### `blusys_wifi_disconnect_reason_t`

```c
typedef enum {
    BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN,
    BLUSYS_WIFI_DISCONNECT_REASON_USER_REQUESTED,
    BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED,
    BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND,
    BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED,
    BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST,
} blusys_wifi_disconnect_reason_t;
```

### `blusys_wifi_ip_info_t`

```c
typedef struct {
    char ip[16];       /* dotted-decimal, e.g. "192.168.1.100" */
    char netmask[16];
    char gateway[16];
} blusys_wifi_ip_info_t;
```

### `blusys_wifi_event_info_t`

```c
typedef struct {
    blusys_wifi_disconnect_reason_t disconnect_reason;
    int raw_disconnect_reason;
    int retry_attempt;
    blusys_wifi_ip_info_t ip_info;
} blusys_wifi_event_info_t;
```

`raw_disconnect_reason` carries the underlying ESP-IDF `wifi_err_reason_t` numeric value when available. It is `0` when no driver-provided reason exists, such as before any disconnect or after an explicit user-requested disconnect.

### `blusys_wifi_event_cb_t`

```c
typedef void (*blusys_wifi_event_cb_t)(blusys_wifi_t *wifi,
                                       blusys_wifi_event_t event,
                                       const blusys_wifi_event_info_t *info,
                                       void *user_ctx);
```

## Functions

### `blusys_wifi_open`

```c
blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config,
                               blusys_wifi_t **out_wifi);
```

Initializes the WiFi stack, configures station mode, and starts the driver. Does **not** connect to the AP — call `blusys_wifi_connect()` separately.

Internally calls `nvs_flash_init()`, `esp_netif_init()`, and `esp_event_loop_create_default()`. These are stack singletons; do not call them separately in the same application.

**Parameters:**
- `config` — SSID and password (required)
- `out_wifi` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_INTERNAL` on driver initialization failure.

---

### `blusys_wifi_close`

```c
blusys_err_t blusys_wifi_close(blusys_wifi_t *wifi);
```

Stops and de-initializes the WiFi stack, destroys the network interface, and frees the handle. After this call the handle is invalid.

---

### `blusys_wifi_connect`

```c
blusys_err_t blusys_wifi_connect(blusys_wifi_t *wifi, int timeout_ms);
```

Initiates a connection to the configured AP and blocks until an IP address is assigned or the timeout expires.

**Returns:**
- `BLUSYS_OK` — connected and IP assigned
- `BLUSYS_ERR_TIMEOUT` — `timeout_ms` elapsed before IP was assigned
- `BLUSYS_ERR_IO` — connection was refused or authentication failed
- `BLUSYS_ERR_BUSY` — another connect attempt is already in progress
- Pass `BLUSYS_TIMEOUT_FOREVER` (`-1`) to block indefinitely

---

### `blusys_wifi_disconnect`

```c
blusys_err_t blusys_wifi_disconnect(blusys_wifi_t *wifi);
```

Disconnects from the current AP. Non-blocking — the disconnected event fires asynchronously.

---

### `blusys_wifi_get_ip_info`

```c
blusys_err_t blusys_wifi_get_ip_info(blusys_wifi_t *wifi,
                                      blusys_wifi_ip_info_t *out_info);
```

Fills `out_info` with the current IP address, netmask, and gateway.

**Returns:** `BLUSYS_ERR_INVALID_STATE` if not connected.

---

### `blusys_wifi_get_last_disconnect_reason`

```c
blusys_wifi_disconnect_reason_t blusys_wifi_get_last_disconnect_reason(blusys_wifi_t *wifi);
```

Returns the most recent disconnect reason recorded by the WiFi handle. This is updated for both explicit `disconnect()` calls and link-loss/authentication failures.

---

### `blusys_wifi_get_last_disconnect_reason_raw`

```c
int blusys_wifi_get_last_disconnect_reason_raw(blusys_wifi_t *wifi);
```

Returns the most recent raw ESP-IDF disconnect reason code recorded by the WiFi handle. Returns `0` when no driver reason is available.

---

### `blusys_wifi_is_connected`

```c
bool blusys_wifi_is_connected(blusys_wifi_t *wifi);
```

Returns `true` if the station currently has an IP address. Non-blocking.

## Lifecycle

```text
blusys_wifi_open()
    └─ started event
blusys_wifi_connect()
    └─ connecting event
       └─ connected event
          └─ got_ip event

unexpected link drop
    └─ disconnected event
       └─ reconnecting event (if auto_reconnect enabled)

blusys_wifi_disconnect()
    └─ disconnected event (USER_REQUESTED)

blusys_wifi_close()
    └─ stopped event
```

## Auto-Reconnect

- disabled by default
- only runs after an unexpected disconnect from a previously connected session
- does not run after explicit `blusys_wifi_disconnect()`
- `max_retries = 0` disables retries
- `max_retries = -1` retries forever

## Callback Notes

- callbacks are asynchronous and should stay short
- use your own task or queue for heavier work
- callbacks are informational; do not call `blusys_wifi_connect()`, `blusys_wifi_disconnect()`, or `blusys_wifi_close()` from them

## Example App

See `examples/reference/wifi_connect/`.
