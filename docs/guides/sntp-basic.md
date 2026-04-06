# Synchronize System Time with NTP

## Problem Statement

You want to set the ESP32's system clock to the correct wall-clock time after boot, so that TLS certificate validation and timestamped logging work correctly.

## Prerequisites

- a supported board
- WiFi access to an NTP server (typically `pool.ntp.org`)

## Minimal Example

```c
blusys_wifi_t *wifi;
blusys_sntp_t *sntp;

/* connect WiFi first */
blusys_wifi_sta_config_t wifi_cfg = { .ssid = "MyNetwork", .password = "secret" };
blusys_wifi_open(&wifi_cfg, &wifi);
blusys_wifi_connect(wifi, 10000);

/* sync time */
blusys_sntp_config_t sntp_cfg = { .server = "pool.ntp.org" };
blusys_sntp_open(&sntp_cfg, &sntp);
blusys_sntp_sync(sntp, 15000);   /* block up to 15 s */

/* now time() returns real wall-clock time */
time_t now;
time(&now);
printf("UTC: %s", ctime(&now));

blusys_sntp_close(sntp);
```

## APIs Used

- `blusys_sntp_open()` initializes the SNTP client and begins polling the NTP server
- `blusys_sntp_sync()` blocks until the system clock is synchronized
- `blusys_sntp_is_synced()` checks sync status without blocking
- `blusys_sntp_close()` stops the client and frees the handle

## Why SNTP Matters

After a cold boot the ESP32 clock starts at epoch (January 1, 1970). This causes:

- TLS certificate validation failures (certificates appear expired or not-yet-valid)
- Incorrect timestamps in logs and sensor readings
- OTA update checks against HTTPS endpoints may silently fail

Call `blusys_sntp_sync()` after WiFi connects and before using `blusys_http_client`, `blusys_mqtt`, or `blusys_ota` with TLS.

## Smooth vs Immediate Sync

By default, `smooth_sync` is `false` — the clock jumps immediately to the correct time. Set `smooth_sync = true` if your application cannot tolerate time jumps (e.g. it measures elapsed intervals). In smooth mode, the clock is gradually adjusted over several minutes.

## Timezones

SNTP sets the clock to UTC. To display local time, use the standard C `setenv` / `tzset` pattern:

```c
setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
tzset();

struct tm local;
localtime_r(&now, &local);
```

## Common Mistakes

- calling `blusys_sntp_open()` before WiFi is connected — the NTP request cannot reach the server
- not waiting for `sync()` to complete before making HTTPS requests — TLS may reject valid certificates
- opening multiple SNTP instances — only one is allowed at a time; the second `open()` returns `BLUSYS_ERR_INVALID_STATE`

## Example App

See `examples/sntp_basic/` for a runnable example.
