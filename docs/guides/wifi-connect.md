# Connect to a WiFi Access Point

## Problem Statement

You want to connect the ESP32 to an existing WiFi access point and get an IP address.

## Prerequisites

- a supported board
- the SSID and password of a 2.4 GHz access point

## Minimal Example

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

## APIs Used

- `blusys_wifi_open()` initializes the WiFi stack and configures STA mode
- `blusys_wifi_connect()` blocks until an IP is assigned or the timeout expires
- `blusys_wifi_get_ip_info()` returns the assigned IP, netmask, and gateway
- `blusys_wifi_disconnect()` disconnects from the AP
- `blusys_wifi_close()` shuts down the WiFi stack and frees the handle
- optional event callbacks report async state changes and reconnect attempts

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

## Expected Runtime Behavior

- `open()` starts the WiFi driver; no RF activity until `connect()` is called
- `connect()` triggers association and DHCP; it returns when `IP_EVENT_STA_GOT_IP` fires
- if the AP is unreachable or the password is wrong, `connect()` returns `BLUSYS_ERR_IO`
- if DHCP does not complete within `timeout_ms`, `connect()` returns `BLUSYS_ERR_TIMEOUT`
- if another connect attempt is already running, `connect()` returns `BLUSYS_ERR_BUSY`
- `is_connected()` can be polled at any time to check current connection state
- when `auto_reconnect` is enabled, unexpected disconnects trigger background reconnect attempts after `reconnect_delay_ms`

## Event Callback

You can subscribe to WiFi lifecycle events without changing the blocking `connect()` flow:

```c
static void on_wifi_event(blusys_wifi_t *wifi, blusys_wifi_event_t event,
                          const blusys_wifi_event_info_t *info, void *user_ctx)
{
    (void) wifi;
    (void) user_ctx;

    if ((event == BLUSYS_WIFI_EVENT_DISCONNECTED) && (info != NULL)) {
        printf("disconnect reason=%d\n", (int)info->disconnect_reason);
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

## NVS Note

`open()` calls `nvs_flash_init()` internally for RF calibration data. Do not call `nvs_flash_init()` separately in the same application — the call is idempotent when the partition already exists.

## Common Mistakes

- calling `get_ip_info()` before `connect()` succeeds — returns `BLUSYS_ERR_INVALID_STATE`
- using a 5 GHz SSID — ESP32 radio supports 2.4 GHz only
- forgetting to call `close()` — the NVS partition and TCP/IP stack are not released until `close()`
- doing heavy work or WiFi lifecycle calls inside the callback — keep callbacks short, non-blocking, and informational only

## Example App

See `examples/wifi_connect/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [WiFi API Reference](../modules/wifi.md).
