# OTA

Over-the-Air firmware update: download a firmware binary from a URL and flash it to the next OTA partition.


## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_ota_config_t`

```c
typedef void (*blusys_ota_progress_cb_t)(uint8_t percent, void *user_ctx);

typedef struct {
    const char *url;        /* firmware URL; required; HTTP or HTTPS */
    const char *cert_pem;   /* server CA certificate (PEM); NULL disables TLS verification */
    int         timeout_ms; /* per-operation network timeout; 0 = default (30 000 ms) */
    blusys_ota_progress_cb_t on_progress; /* optional; NULL to ignore */
    void        *progress_ctx;            /* passed to on_progress */
} blusys_ota_config_t;
```

`url` and `cert_pem` pointers must remain valid until `blusys_ota_perform()` returns.

If `on_progress` is non-NULL, it may be called repeatedly during `blusys_ota_perform()` with `percent` in the range 0–100 (implementation-defined frequency). Use `progress_ctx` for your own state.

## Functions

### `blusys_ota_open`

```c
blusys_err_t blusys_ota_open(const blusys_ota_config_t *config, blusys_ota_t **out_handle);
```

Allocates an OTA session handle.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if config, out_handle, or config->url is NULL, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.

---

### `blusys_ota_perform`

```c
blusys_err_t blusys_ota_perform(blusys_ota_t *handle);
```

Downloads firmware from the configured URL and writes it to the next OTA partition. Blocks until the download and flash are complete or an error occurs. On success the new partition is staged as the next boot target. On failure the current firmware is unaffected.

**Returns:**
- `BLUSYS_OK` — firmware downloaded and staged successfully
- `BLUSYS_ERR_INVALID_ARG` — NULL handle
- `BLUSYS_ERR_IO` — network error or incomplete download
- `BLUSYS_ERR_NO_MEM` — allocation failure
- `BLUSYS_ERR_FAIL` — firmware validation failed or partition error

---

### `blusys_ota_close`

```c
blusys_err_t blusys_ota_close(blusys_ota_t *handle);
```

Frees the OTA handle. Call after `blusys_ota_perform()` whether it succeeded or failed.

---

### `blusys_ota_restart`

```c
void blusys_ota_restart(void);
```

Restarts the device to boot the new firmware. Call after a successful `blusys_ota_close()`.

---

### `blusys_ota_mark_valid`

```c
blusys_err_t blusys_ota_mark_valid(void);
```

Marks the currently running firmware as valid, cancelling any pending rollback. Call once after booting into new firmware and verifying it works correctly. Only needed if the partition table is configured with rollback support.

## Lifecycle

```
WiFi connected
    └── blusys_ota_open()
            └── blusys_ota_perform()   [blocks; downloads + flashes]
        blusys_ota_close()
        blusys_ota_restart()           [device reboots into new firmware]
            └── (optional) blusys_ota_mark_valid()   [cancel rollback]
```

The WiFi connection must be established before calling `open()`.

## Thread Safety

`open()` and `close()` must not be called concurrently on the same handle. `perform()` is blocking and must not be called concurrently with itself on the same handle. `restart()` and `mark_valid()` are stateless.

## Limitations

- The WiFi connection must be established before calling `open()`.
- `perform()` blocks the calling task for the duration of the download. For large firmware images on slow connections, use a dedicated task with sufficient stack.
- HTTPS requires either a valid `cert_pem` or a server certificate signed by a trusted CA. Passing `NULL` for `cert_pem` disables TLS verification and is suitable for plain HTTP only.
- The partition table must include at least one OTA partition (`ota_0`). The default ESP-IDF partition table does not include OTA partitions — use `partitions_two_ota.csv` or a custom table.

## Example App

See `examples/validation/network_services/` (`net_ota` scenario).
