# OTA

Over-the-Air firmware update: download a firmware binary from a URL, flash it to the next OTA partition, then reboot into the new image. Optional SHA256 integrity verification and IDF rollback support.

## Quick Example

```c
#include "blusys/blusys.h"

static void on_progress(uint8_t percent, void *ctx)
{
    (void)ctx;
    printf("OTA progress: %u%%\n", percent);
}

void run_ota(void)
{
    blusys_ota_config_t cfg = {
        .url         = "https://example.com/firmware.bin",
        .cert_pem    = server_ca_pem,
        .timeout_ms  = 30000,
        .on_progress = on_progress,
    };

    blusys_ota_t *ota;
    if (blusys_ota_open(&cfg, &ota) != BLUSYS_OK) return;

    blusys_err_t err = blusys_ota_perform(ota);
    blusys_ota_close(ota);

    if (err == BLUSYS_OK) {
        blusys_ota_restart();   /* does not return */
    }
}

/* After the first successful boot on the new firmware: */
blusys_ota_mark_valid();
```

## Common Mistakes

- **running without WiFi connected** — establish the network before calling `open()`
- **no OTA partitions in the partition table** — the default ESP-IDF partition table has no OTA slots; use `partitions_two_ota.csv` or a custom table
- **disabling TLS verification for HTTPS** — passing `cert_pem = NULL` skips verification; pin a CA cert for production
- **forgetting `mark_valid()`** — if the partition table enables rollback, the bootloader reverts to the previous image unless `mark_valid()` is called after a successful boot

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- `open()` and `close()` must not be called concurrently on the same handle
- `perform()` blocks and must not be called concurrently with itself on the same handle
- `restart()` and `mark_valid()` are stateless

## ISR Notes

No ISR-safe calls are defined for the OTA module.

## Limitations

- requires an established WiFi connection before `open()`
- `perform()` blocks the calling task for the duration of the download — use a dedicated task with sufficient stack for large firmware on slow connections
- HTTPS requires either a valid `cert_pem` or a server certificate signed by a trusted CA; `NULL` is only suitable for plain HTTP
- the partition table must include at least one OTA partition (`ota_0`)

## Example App

See `examples/validation/network_services/` (`net_ota` scenario).
