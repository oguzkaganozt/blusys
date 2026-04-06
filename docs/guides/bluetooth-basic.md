# Advertise and Scan with BLE

## Problem Statement

You want the ESP32 to announce itself over Bluetooth Low Energy so nearby phones or devices can discover it, or to scan for other BLE devices in the vicinity.

## Prerequisites

- A supported board (ESP32, ESP32-C3, or ESP32-S3)
- `CONFIG_BT_ENABLED=y` and `CONFIG_BT_NIMBLE_ENABLED=y` in sdkconfig (see [sdkconfig Requirements](#sdkconfig-requirements))

## Advertise (Peripheral) Example

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

## Scan (Central) Example

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

## APIs Used

- `blusys_bluetooth_open()` — initializes the NimBLE stack; blocks until sync (up to 5 s)
- `blusys_bluetooth_advertise_start()` — starts undirected general BLE advertising
- `blusys_bluetooth_advertise_stop()` — stops advertising
- `blusys_bluetooth_scan_start()` — starts passive scanning; fires callback per device found
- `blusys_bluetooth_scan_stop()` — stops scanning
- `blusys_bluetooth_close()` — shuts down the stack and frees the handle

## Address Format

`blusys_bluetooth_scan_result_t.addr` stores the 6 address bytes in BLE wire order (index 0 is the least significant byte). To print the conventional human-readable format (MSB first), iterate in reverse:

```c
printf("%02x:%02x:%02x:%02x:%02x:%02x",
       result->addr[5], result->addr[4], result->addr[3],
       result->addr[2], result->addr[1], result->addr[0]);
```

## sdkconfig Requirements

BLE requires explicit opt-in in menuconfig. The easiest way is to add a `sdkconfig.defaults` file to your project:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
```

On **ESP32 only**, also set the BT controller mode (ESP32-C3 and ESP32-S3 are BLE-only in hardware and need no additional setting):

```
CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y
```

The `examples/bluetooth_basic/` example provides these files pre-configured.

## Common Mistakes

- **Calling `open` twice without `close`** — returns `BLUSYS_ERR_INVALID_STATE`. NimBLE is a singleton stack; only one handle is allowed at a time.
- **Calling `advertise_start` again while already advertising** — returns `BLUSYS_ERR_INVALID_STATE`; call `advertise_stop` first.
- **Device name longer than 29 bytes** — the BLE advertising payload is 31 bytes and the name AD structure uses 2 bytes of overhead; longer names get truncated by the controller.
- **Missing `CONFIG_BT_ENABLED=y`** — the build will fail with undefined references to NimBLE symbols.
- **Calling `scan_stop` or `close` from inside the scan callback** — the callback runs in the NimBLE host task; calling these from within it will deadlock. Signal a FreeRTOS task or event group instead.

## Example App

See `examples/bluetooth_basic/` for a runnable example. Use the `BT_MODE` Kconfig option to switch between advertise and scan modes.

## API Reference

For full type definitions and function signatures, see [Bluetooth API Reference](../modules/bluetooth.md).
