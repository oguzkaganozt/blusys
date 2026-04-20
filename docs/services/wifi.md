# WiFi

Station-mode WiFi for connecting the ESP32 to an existing access point.

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

`open()` and `connect()` are deliberately separate so you can initialize the stack at boot and defer the connection attempt until needed:

```c
/* init at boot */
blusys_wifi_open(&cfg, &wifi);

/* connect when required */
if (need_network) {
    blusys_wifi_connect(wifi, 10000);
}
```

## Event Callback

Subscribe to WiFi lifecycle events without changing the blocking `connect()` flow:

```c
static void on_wifi_event(blusys_wifi_t *wifi, blusys_wifi_event_t event,
                          const blusys_wifi_event_info_t *info, void *user_ctx)
{
    if ((event == BLUSYS_WIFI_EVENT_DISCONNECTED) && (info != NULL)) {
        printf("disconnect reason=%d raw=%d\n",
               (int)info->disconnect_reason,
               info->raw_disconnect_reason);
    }
}

blusys_wifi_sta_config_t cfg = {
    .ssid     = "MyNetwork",
    .password = "MyPassword",
    .on_event = on_wifi_event,
};
```

Keep callbacks short. Hand off long-running work to your own FreeRTOS task or queue. Treat callbacks as informational only: do not call `blusys_wifi_connect()`, `blusys_wifi_disconnect()`, or `blusys_wifi_close()` from them.

If you need the exact driver reason for diagnostics, read `info->raw_disconnect_reason` in the callback or call `blusys_wifi_get_last_disconnect_reason_raw()` after `connect()` fails.

## Auto-Reconnect

- disabled by default
- only runs after an unexpected disconnect from a previously connected session
- does not run after explicit `blusys_wifi_disconnect()`
- `max_retries = 0` disables retries; `max_retries = -1` retries forever

## Common Mistakes

- calling `get_ip_info()` before `connect()` succeeds — returns `BLUSYS_ERR_INVALID_STATE`
- using a 5 GHz SSID — ESP32 radio supports 2.4 GHz only
- forgetting to call `close()` — the NVS partition and TCP/IP stack are not released until `close()`
- doing heavy work or WiFi lifecycle calls inside the callback — keep callbacks short, non-blocking, and informational only

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

Only one WiFi owner may be active at a time — do not combine with `blusys_espnow`, `blusys_wifi_mesh`, or `blusys_wifi_prov`.

## Thread Safety

All functions are thread-safe. The `on_event` callback runs on the ESP event task — do not call `connect()`, `disconnect()`, or `close()` from within it.

## Limitations

- 2.4 GHz only
- station mode only (no SoftAP or AP+STA)
- no WPA3-SAE enterprise; WPA2-PSK and open networks are the tested paths

## Example App

See `examples/reference/connectivity/`.
