# OTA

Over-the-air firmware update: download, flash the next slot, reboot.

## At a glance

- requires a working network path
- `perform()` blocks
- mark the new image valid after first boot

## Quick example

```c
blusys_ota_config_t cfg = {
    .url = "https://example.com/firmware.bin",
    .cert_pem = server_ca_pem,
    .timeout_ms = 30000,
};

blusys_ota_t *ota;
if (blusys_ota_open(&cfg, &ota) == BLUSYS_OK) {
    if (blusys_ota_perform(ota) == BLUSYS_OK) {
        blusys_ota_restart();
    }
    blusys_ota_close(ota);
}
```

## Common mistakes

- running without WiFi connected
- missing OTA partitions
- disabling TLS verification for production HTTPS
- forgetting `mark_valid()` when rollback is enabled

## Target support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread safety

- open and close must not overlap
- `perform()` is single-flight per handle

## Limitations

- requires an OTA partition table
- HTTPS needs a valid certificate chain

## Example app

`examples/validation/network_services/` (`net_ota`)
