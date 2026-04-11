# SNTP

NTP time synchronization for setting the system clock after boot.


## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_sntp_config_t`

```c
typedef struct {
    const char             *server;       /* Primary NTP server (default: "pool.ntp.org") */
    const char             *server2;      /* Optional secondary server (NULL to skip) */
    bool                    smooth_sync;  /* true = gradual adjustment, false = immediate */
    blusys_sntp_sync_cb_t   sync_cb;      /* Optional callback on each sync event */
    void                   *user_ctx;     /* User context passed to sync_cb */
} blusys_sntp_config_t;
```

### `blusys_sntp_sync_cb_t`

```c
typedef void (*blusys_sntp_sync_cb_t)(void *user_ctx);
```

Callback invoked each time the system clock is synchronized with the NTP server.

## Functions

### `blusys_sntp_open`

```c
blusys_err_t blusys_sntp_open(const blusys_sntp_config_t *config,
                               blusys_sntp_t **out_sntp);
```

Initializes the SNTP client and begins polling the configured NTP server(s). WiFi must already be connected before calling this function.

Only one SNTP instance may be active at a time.

**Parameters:**

- `config` — server(s) and options (required)
- `out_sntp` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if already open, `BLUSYS_ERR_NO_MEM` if allocation fails.

---

### `blusys_sntp_close`

```c
blusys_err_t blusys_sntp_close(blusys_sntp_t *sntp);
```

Stops the SNTP client and frees the handle. The system clock retains the last synchronized time.

---

### `blusys_sntp_sync`

```c
blusys_err_t blusys_sntp_sync(blusys_sntp_t *sntp, int timeout_ms);
```

Blocks until the system clock has been synchronized with the NTP server or the timeout expires.

After this function returns `BLUSYS_OK`, standard C functions (`time()`, `localtime()`, `gmtime()`) return correct wall-clock time.

**Parameters:**

- `sntp` — handle from `blusys_sntp_open()`
- `timeout_ms` — maximum wait time; pass `BLUSYS_TIMEOUT_FOREVER` (`-1`) to block indefinitely

**Returns:**

- `BLUSYS_OK` — clock synchronized
- `BLUSYS_ERR_TIMEOUT` — `timeout_ms` elapsed before sync completed

---

### `blusys_sntp_is_synced`

```c
bool blusys_sntp_is_synced(blusys_sntp_t *sntp);
```

Returns `true` if the system clock has been synchronized at least once. Non-blocking.
