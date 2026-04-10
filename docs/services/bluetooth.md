# Bluetooth

BLE advertising and scanning for ESP32 devices.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note
    This module supports BLE (Bluetooth Low Energy) only. Classic Bluetooth (BR/EDR) is not exposed. The underlying stack is NimBLE, which must be enabled in sdkconfig — see [Stack Notes](#stack-notes).

!!! warning
    `blusys_bluetooth` owns the BLE host stack while open. It cannot be open at the same time as `blusys_ble_gatt`, BLE transport `blusys_usb_hid`, or BLE transport `blusys_wifi_prov`.

## Quick Example

### Advertise (Peripheral)

```c
blusys_bluetooth_t *bt = NULL;
blusys_bluetooth_config_t cfg = { .device_name = "my-sensor" };

blusys_bluetooth_open(&cfg, &bt);
blusys_bluetooth_advertise_start(bt);

/* device is now visible to BLE scanners as "my-sensor" */

/* ... later ... */
blusys_bluetooth_advertise_stop(bt);
blusys_bluetooth_close(bt);
```

### Scan (Central)

```c
static void scan_cb(const blusys_bluetooth_scan_result_t *result, void *user_ctx)
{
    printf("Found: %s  rssi=%d dBm\n",
           result->name[0] ? result->name : "(no name)", result->rssi);
}

blusys_bluetooth_t *bt = NULL;
blusys_bluetooth_config_t cfg = { .device_name = "my-scanner" };

blusys_bluetooth_open(&cfg, &bt);
blusys_bluetooth_scan_start(bt, scan_cb, NULL);

vTaskDelay(pdMS_TO_TICKS(30000));   /* scan for 30 s */

blusys_bluetooth_scan_stop(bt);
blusys_bluetooth_close(bt);
```

## Common Mistakes

- **Calling `open` twice without `close`** — returns `BLUSYS_ERR_INVALID_STATE`. NimBLE is a singleton stack; only one handle is allowed at a time.
- **Calling `advertise_start` again while already advertising** — returns `BLUSYS_ERR_INVALID_STATE`; call `advertise_stop` first.
- **Device name longer than 29 bytes** — `blusys_bluetooth_open()` returns `BLUSYS_ERR_INVALID_ARG`.
- **Missing `CONFIG_BT_ENABLED=y`** — the build will fail with undefined references to NimBLE symbols.
- **Missing NimBLE roles for the selected mode** — advertising requires peripheral + broadcaster; scanning requires central + observer.
- **Calling `scan_stop` or `close` from inside the scan callback** — the callback runs in the NimBLE host task; calling these from within it will deadlock. Signal a FreeRTOS task or event group instead.

## Types

### `blusys_bluetooth_config_t`

```c
typedef struct {
    const char *device_name;  /**< BLE GAP name broadcast in advertising data */
} blusys_bluetooth_config_t;
```

Configuration passed to `blusys_bluetooth_open()`. `device_name` is required and must not be NULL. It must be 29 bytes or shorter — longer names are rejected with `BLUSYS_ERR_INVALID_ARG`.

### `blusys_bluetooth_scan_result_t`

```c
typedef struct {
    char    name[32];   /**< Device name from advertising payload; empty string if not present */
    uint8_t addr[6];    /**< BLE address bytes, wire order (LSB first) */
    int8_t  rssi;       /**< Received signal strength in dBm */
} blusys_bluetooth_scan_result_t;
```

Delivered to the scan callback for each advertising device found. `name` is empty if the device does not include a Complete Local Name AD structure. `addr` bytes are in BLE wire order (index 0 is LSB); to print the conventional human-readable format, iterate from index 5 down to 0.

### `blusys_bluetooth_scan_cb_t`

```c
typedef void (*blusys_bluetooth_scan_cb_t)(const blusys_bluetooth_scan_result_t *result,
                                            void *user_ctx);
```

Callback invoked from the NimBLE host task for each advertising report received during a scan. The `result` pointer is valid only for the duration of the callback — copy any fields you need before returning. `user_ctx` is the pointer passed to `blusys_bluetooth_scan_start()`.

## Functions

### `blusys_bluetooth_open`

```c
blusys_err_t blusys_bluetooth_open(const blusys_bluetooth_config_t *config,
                                    blusys_bluetooth_t **out_bt);
```

Initializes the NimBLE BLE stack and returns a handle. Only one handle may be open at a time — a second call returns `BLUSYS_ERR_INVALID_STATE` until `blusys_bluetooth_close()` is called. Blocks up to 5 s waiting for the NimBLE host to synchronize with the controller.

**Parameters:**

- `config` — device name (required)
- `out_bt` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` for invalid configuration, `BLUSYS_ERR_INVALID_STATE` if already open or another BLE-owning module is already open, `BLUSYS_ERR_TIMEOUT` if the stack does not synchronize within 5 s, `BLUSYS_ERR_NO_MEM` on allocation failure.

---

### `blusys_bluetooth_close`

```c
blusys_err_t blusys_bluetooth_close(blusys_bluetooth_t *bt);
```

Stops advertising and scanning if active, shuts down the NimBLE stack and BT controller, and frees the handle. The handle must not be used after this call.

---

### `blusys_bluetooth_advertise_start`

```c
blusys_err_t blusys_bluetooth_advertise_start(blusys_bluetooth_t *bt);
```

Starts undirected general BLE advertising using the device name set in `blusys_bluetooth_open()`. The device becomes visible to nearby BLE scanners. Advertising continues indefinitely until `blusys_bluetooth_advertise_stop()` or `blusys_bluetooth_close()` is called.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if already advertising.

---

### `blusys_bluetooth_advertise_stop`

```c
blusys_err_t blusys_bluetooth_advertise_stop(blusys_bluetooth_t *bt);
```

Stops BLE advertising.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if not currently advertising.

---

### `blusys_bluetooth_scan_start`

```c
blusys_err_t blusys_bluetooth_scan_start(blusys_bluetooth_t *bt,
                                          blusys_bluetooth_scan_cb_t cb,
                                          void *user_ctx);
```

Starts passive BLE scanning. For each advertising device found, `cb` is invoked from the NimBLE host task with the device's name, address, and RSSI. Duplicate reports from the same address are suppressed. Scanning continues until `blusys_bluetooth_scan_stop()` or `blusys_bluetooth_close()` is called.

**Parameters:**

- `cb` — scan event callback (required, must not be NULL)
- `user_ctx` — passed unchanged to each `cb` invocation

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if already scanning.

---

### `blusys_bluetooth_scan_stop`

```c
blusys_err_t blusys_bluetooth_scan_stop(blusys_bluetooth_t *bt);
```

Stops BLE scanning.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if not currently scanning.

## Lifecycle

```
blusys_bluetooth_open()
    │
    ├─ blusys_bluetooth_advertise_start()
    │       └─ blusys_bluetooth_advertise_stop()
    │
    ├─ blusys_bluetooth_scan_start()
    │       └─ blusys_bluetooth_scan_stop()
    │
blusys_bluetooth_close()
```

Advertising and scanning can run simultaneously on the same handle.

## Thread Safety

All functions are thread-safe. The scan callback is invoked from the NimBLE host task — do not call `blusys_bluetooth_scan_stop()` or `blusys_bluetooth_close()` from within the callback.

## Stack Notes

NimBLE must be enabled via sdkconfig before building any project that uses this module:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_CENTRAL=y
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y
CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y
CONFIG_BT_NIMBLE_ROLE_OBSERVER=y
```

On ESP32, also set:

```
CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y
```

The `examples/validation/bluetooth_basic/` example provides `sdkconfig.defaults` and `sdkconfig.esp32` with these values preset.

## Limitations

- BLE only — no Classic Bluetooth (BR/EDR)
- No GATT server or client in this cut
- No connection establishment or management
- Passive scan only — no active scan requests (scan response data not fetched)
- One active BLE-owning service at a time (`bluetooth`, `ble_gatt`, BLE `usb_hid`, BLE `wifi_prov`)

## Example App

See `examples/validation/bluetooth_basic/`.
