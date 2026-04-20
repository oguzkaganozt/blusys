# Bluetooth

BLE advertising and scanning for ESP32 devices.

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

- **calling `open` twice without `close`** — returns `BLUSYS_ERR_INVALID_STATE`. NimBLE is a singleton stack; only one handle is allowed at a time.
- **calling `advertise_start` again while already advertising** — returns `BLUSYS_ERR_INVALID_STATE`; call `advertise_stop` first.
- **device name longer than 29 bytes** — `blusys_bluetooth_open()` returns `BLUSYS_ERR_INVALID_ARG`.
- **missing `CONFIG_BT_ENABLED=y`** — the build fails with undefined references to NimBLE symbols.
- **missing NimBLE roles for the selected mode** — advertising requires peripheral + broadcaster; scanning requires central + observer.
- **calling `scan_stop` or `close` from inside the scan callback** — the callback runs in the NimBLE host task; calling these from within it deadlocks. Signal a FreeRTOS task or event group instead.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note
    BLE only. Classic Bluetooth (BR/EDR) is not exposed.

!!! warning
    `blusys_bluetooth` owns the BLE host stack while open. It cannot be open at the same time as `blusys_ble_gatt`, BLE-transport `blusys_usb_hid`, BLE-transport `blusys_wifi_prov`, or `blusys_ble_hid_device`.

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

The `examples/validation/wireless_bt_lab` example provides `sdkconfig.defaults` and `sdkconfig.esp32` with these values preset.

## Limitations

- BLE only — no Classic Bluetooth (BR/EDR)
- No GATT server or client in this module (see `blusys_ble_gatt`)
- No connection establishment or management
- Passive scan only — no active scan requests (scan-response data not fetched)
- One active BLE-owning service at a time

## Example App

See `examples/validation/wireless_bt_lab`.
