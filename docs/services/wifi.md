# WiFi

Station-mode WiFi. Use it to bring the SoC onto the network; higher-level clients depend on it.

## At a glance

- one WiFi owner at a time
- connect is blocking, open and close are separate
- callbacks are informational only

## Quick example

```c
blusys_wifi_t *wifi;
blusys_wifi_sta_config_t cfg = {
    .ssid = "MyNetwork",
    .password = "MyPassword",
    .auto_reconnect = true,
};

blusys_wifi_open(&cfg, &wifi);
blusys_wifi_connect(wifi, 10000);
blusys_wifi_close(wifi);
```

## Common mistakes

- calling `connect()` or `close()` from the WiFi callback
- mixing WiFi with ESP-NOW, mesh, or provisioning on the same build
- reading IP info before `connect()` succeeds

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- all functions are thread-safe
- the event callback runs on the ESP event task
- keep callbacks short and non-blocking

## Limitations

- 2.4 GHz only
- station mode only
- no WPA3-SAE enterprise path

## Example app

`examples/reference/connectivity/`
