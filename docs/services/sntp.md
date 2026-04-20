# SNTP

NTP time synchronization for setting the system clock after boot.

## Quick Example

```c
#include <time.h>
#include "blusys/blusys.h"

void app_main(void)
{
    /* WiFi must already be connected. */

    blusys_sntp_t *sntp;
    blusys_sntp_config_t cfg = {
        .server      = "pool.ntp.org",
        .smooth_sync = false,
    };
    blusys_sntp_open(&cfg, &sntp);

    if (blusys_sntp_sync(sntp, 10000) == BLUSYS_OK) {
        time_t now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);
        printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
               tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
}
```

## Common Mistakes

- **calling `open()` before WiFi has an IP** — `sync()` will time out; bring up WiFi first
- **opening a second SNTP instance** — the ESP-IDF backend is a singleton; close the existing instance before opening a new one
- **blocking inside `sync_cb`** — the callback runs on the SNTP service task

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- `open()` / `close()` are not reentrant
- `is_synced()` is safe from any task
- `sync_cb` runs on the SNTP service task — do not block

## ISR Notes

No ISR-safe calls are defined for the SNTP module.

## Limitations

- single active instance
- requires WiFi (or another network interface with routable DNS) before `open()`
- the system clock retains the last sync across `close()` / `open()` cycles; there is no "reset to epoch" helper

## Example App

See `examples/validation/network_services/` (`net_sntp` scenario).
