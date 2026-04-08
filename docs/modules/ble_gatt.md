# BLE GATT Server

BLE GATT server for exposing sensor data and accepting commands over Bluetooth Low Energy.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [BLE GATT Basics](../guides/ble_gatt-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note
    This module supports BLE (Bluetooth Low Energy) only via the NimBLE stack. Classic Bluetooth (BR/EDR) is not exposed. NimBLE must be enabled in sdkconfig — see [Stack Notes](#stack-notes).

!!! warning
    `blusys_ble_gatt` owns the BLE host stack while open. It cannot be open at the same time as `blusys_bluetooth`, BLE transport `blusys_usb_hid`, or BLE transport `blusys_wifi_prov`.

## Types

### `BLUSYS_BLE_GATT_CHR_F_*`

```c
#define BLUSYS_BLE_GATT_CHR_F_READ    (1u << 0)
#define BLUSYS_BLE_GATT_CHR_F_WRITE   (1u << 1)
#define BLUSYS_BLE_GATT_CHR_F_NOTIFY  (1u << 2)
```

Characteristic property flags combined into the `flags` field of `blusys_ble_gatt_chr_def_t`. When `BLUSYS_BLE_GATT_CHR_F_NOTIFY` is set, NimBLE automatically adds a Client Characteristic Configuration Descriptor (CCCD) so clients can subscribe.

### `blusys_ble_gatt_read_cb_t`

```c
typedef int (*blusys_ble_gatt_read_cb_t)(uint16_t conn_handle,
                                          const uint8_t uuid[16],
                                          uint8_t *out_data, size_t max_len,
                                          size_t *out_len, void *user_ctx);
```

Called from the NimBLE host task when a client reads the characteristic. Write up to `max_len` bytes into `out_data` and set `*out_len` to the actual number of bytes. Return `0` on success or a NimBLE ATT error code on failure.

### `blusys_ble_gatt_write_cb_t`

```c
typedef int (*blusys_ble_gatt_write_cb_t)(uint16_t conn_handle,
                                           const uint8_t uuid[16],
                                           const uint8_t *data, size_t len,
                                           void *user_ctx);
```

Called from the NimBLE host task when a client writes the characteristic. `data` and `len` hold the client-provided payload. Return `0` on success or an ATT error code on failure.

### `blusys_ble_gatt_conn_cb_t`

```c
typedef void (*blusys_ble_gatt_conn_cb_t)(uint16_t conn_handle, bool connected,
                                           void *user_ctx);
```

Optional callback invoked when a client connects (`connected = true`) or disconnects (`connected = false`). `conn_handle` identifies the connection and is the value to pass to `blusys_ble_gatt_notify()`. Do not call `blusys_ble_gatt_close()` from within this callback.

### `blusys_ble_gatt_chr_def_t`

```c
typedef struct {
    uint8_t                      uuid[16];
    uint32_t                     flags;
    blusys_ble_gatt_read_cb_t    read_cb;
    blusys_ble_gatt_write_cb_t   write_cb;
    void                        *user_ctx;
    uint16_t                    *val_handle_out;
} blusys_ble_gatt_chr_def_t;
```

Describes one characteristic in a GATT service.

- `uuid` — 128-bit UUID in little-endian byte order (index 0 is the least significant byte).
- `flags` — bitwise OR of `BLUSYS_BLE_GATT_CHR_F_*` values.
- `read_cb` — required when `F_READ` is set; called for each client read.
- `write_cb` — required when `F_WRITE` is set; called for each client write.
- `user_ctx` — passed to `read_cb` and `write_cb` unchanged.
- `val_handle_out` — pointer to a `uint16_t` that is filled by `blusys_ble_gatt_open()` with the NimBLE attribute handle. **Required** when `F_NOTIFY` is set — pass this value to `blusys_ble_gatt_notify()`.

The array of `blusys_ble_gatt_chr_def_t` and all fields within it must remain valid for the lifetime of the GATT handle.

### `blusys_ble_gatt_svc_def_t`

```c
typedef struct {
    uint8_t                           uuid[16];
    const blusys_ble_gatt_chr_def_t  *characteristics;
    size_t                            chr_count;
} blusys_ble_gatt_svc_def_t;
```

Groups one or more characteristics under a primary GATT service.

- `uuid` — 128-bit service UUID, little-endian.
- `characteristics` — pointer to an array of `chr_count` characteristic definitions.
- `chr_count` — number of entries in `characteristics`.

### `blusys_ble_gatt_config_t`

```c
typedef struct {
    const char                       *device_name;
    const blusys_ble_gatt_svc_def_t  *services;
    size_t                            svc_count;
    blusys_ble_gatt_conn_cb_t         conn_cb;
    void                             *conn_user_ctx;
} blusys_ble_gatt_config_t;
```

Passed to `blusys_ble_gatt_open()`. All pointers must remain valid for the lifetime of the handle.

- `device_name` — BLE advertised name; must be 29 bytes or shorter or `open()` returns `BLUSYS_ERR_INVALID_ARG`.
- `services` — array of `svc_count` service definitions.
- `svc_count` — must be at least 1.
- `conn_cb` — optional; receives connect/disconnect events.
- `conn_user_ctx` — passed to `conn_cb` unchanged.

## Functions

### `blusys_ble_gatt_open`

```c
blusys_err_t blusys_ble_gatt_open(const blusys_ble_gatt_config_t *config,
                                   blusys_ble_gatt_t **out_handle);
```

Initializes the NimBLE stack, registers the provided GATT services, and starts BLE advertising. Blocks up to 5 s waiting for the NimBLE host to synchronize with the controller. On return, the device is advertising and ready for client connections.

**Parameters:**

- `config` — server configuration; all pointers must remain valid while the handle is open.
- `out_handle` — receives the allocated handle on success.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` for invalid configuration, `BLUSYS_ERR_INVALID_STATE` if already open or another BLE-owning module is already open, `BLUSYS_ERR_TIMEOUT` if the NimBLE stack does not synchronize within 5 s, `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_INTERNAL` on NimBLE registration or advertising error.

---

### `blusys_ble_gatt_close`

```c
blusys_err_t blusys_ble_gatt_close(blusys_ble_gatt_t *handle);
```

Stops advertising, shuts down the NimBLE stack and BT controller, frees all internal memory, and invalidates the handle.

**Parameters:**

- `handle` — handle returned by `blusys_ble_gatt_open()`.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if handle is NULL.

---

### `blusys_ble_gatt_notify`

```c
blusys_err_t blusys_ble_gatt_notify(blusys_ble_gatt_t *handle,
                                     uint16_t conn_handle,
                                     uint16_t chr_val_handle,
                                     const void *data, size_t len);
```

Sends a BLE notification for a characteristic to a specific connected client. The client must have subscribed by enabling the characteristic's CCCD; if it has not subscribed, NimBLE silently discards the notification.

**Parameters:**

- `handle` — handle returned by `blusys_ble_gatt_open()`.
- `conn_handle` — connection handle received in `conn_cb`.
- `chr_val_handle` — value stored in the `val_handle_out` field of the characteristic definition after `open()`.
- `data` — notification payload.
- `len` — payload length in bytes.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if handle or data is NULL, `BLUSYS_ERR_INVALID_STATE` if the server is closing, `BLUSYS_ERR_NO_MEM` if buffer allocation fails, `BLUSYS_ERR_INTERNAL` on NimBLE error.

## Lifecycle

```
blusys_ble_gatt_open()      ← defines services, starts advertising
    │
    ├─ client connects → conn_cb(connected=true)
    │      │
    │      ├─ read_cb / write_cb  (from NimBLE host task)
    │      ├─ blusys_ble_gatt_notify()
    │      │
    │      └─ client disconnects → conn_cb(connected=false)
    │           └─ advertising restarts automatically
    │
blusys_ble_gatt_close()
```

## Thread Safety

All public functions are thread-safe. The `read_cb`, `write_cb`, and `conn_cb` callbacks are invoked from the NimBLE host task — do not call `blusys_ble_gatt_close()` from within any callback. Signal a separate FreeRTOS task or event group instead.

## Stack Notes

NimBLE and the peripheral role must be enabled in sdkconfig:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y
```

The `examples/ble_gatt_basic/` example provides `sdkconfig.defaults` with these values preset, and `sdkconfig.esp32` also selects BLE-only controller mode on ESP32.

## Limitations

- GATT server (peripheral) role only — no GATT client
- 128-bit UUIDs only — 16-bit Bluetooth SIG UUIDs are not directly supported
- Single active connection tracked for notifications; multi-client notify requires iterating connections at the application level
- Maximum read/write payload handled by the internal buffer: 512 bytes
- One active BLE-owning service at a time (`ble_gatt`, `bluetooth`, BLE `usb_hid`, BLE `wifi_prov`)
