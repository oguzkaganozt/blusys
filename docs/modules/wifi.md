# WiFi

Station-mode WiFi for connecting the ESP32 to an existing access point.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_wifi_sta_config_t`

```c
typedef struct {
    const char *ssid;      /* AP SSID (required) */
    const char *password;  /* AP password, NULL or "" for open networks */
} blusys_wifi_sta_config_t;
```

### `blusys_wifi_ip_info_t`

```c
typedef struct {
    char ip[16];       /* dotted-decimal, e.g. "192.168.1.100" */
    char netmask[16];
    char gateway[16];
} blusys_wifi_ip_info_t;
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

### `blusys_wifi_is_connected`

```c
bool blusys_wifi_is_connected(blusys_wifi_t *wifi);
```

Returns `true` if the station currently has an IP address. Non-blocking.
