# Expose Sensor Data via BLE GATT

## Problem Statement

You want the ESP32 to act as a BLE peripheral — exposing readable/writable characteristics so a phone or PC can read sensor values, subscribe to periodic updates (notifications), or push configuration bytes to the device.

## Prerequisites

- A supported board (ESP32, ESP32-C3, or ESP32-S3)
- `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, and `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y` in sdkconfig (see [sdkconfig Requirements](#sdkconfig-requirements))
- A BLE client for testing — the free **nRF Connect** app (iOS/Android) works well

## Minimal Example

```c
#include <string.h>
#include "blusys/ble_gatt.h"

/* Service UUID: 12345678-1234-1234-1234-1234567890ab (little-endian) */
static const uint8_t SVC_UUID[16] = {
    0xab, 0x90, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12
};

/* Characteristic UUID: 12345678-1234-1234-1234-1234567890cd (little-endian) */
static const uint8_t CHR_UUID[16] = {
    0xcd, 0x90, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12
};

static uint16_t s_val_handle;
static uint32_t s_counter = 0;

static int on_read(uint16_t conn_handle, const uint8_t uuid[16],
                   uint8_t *out_data, size_t max_len,
                   size_t *out_len, void *user_ctx)
{
    uint32_t v = s_counter;
    memcpy(out_data, &v, sizeof(v));
    *out_len = sizeof(v);
    return 0;
}

static void on_conn(uint16_t conn_handle, bool connected, void *user_ctx)
{
    printf("Client %s (conn=%u)\n", connected ? "connected" : "disconnected",
           conn_handle);
}

void app_main(void)
{
    blusys_ble_gatt_chr_def_t chrs[1] = {{
        .flags          = BLUSYS_BLE_GATT_CHR_F_READ | BLUSYS_BLE_GATT_CHR_F_NOTIFY,
        .read_cb        = on_read,
        .val_handle_out = &s_val_handle,
    }};
    memcpy(chrs[0].uuid, CHR_UUID, 16);

    blusys_ble_gatt_svc_def_t svcs[1] = {{
        .characteristics = chrs,
        .chr_count       = 1,
    }};
    memcpy(svcs[0].uuid, SVC_UUID, 16);

    blusys_ble_gatt_config_t cfg = {
        .device_name = "MySensor",
        .services    = svcs,
        .svc_count   = 1,
        .conn_cb     = on_conn,
    };

    blusys_ble_gatt_t *gatt = NULL;
    blusys_ble_gatt_open(&cfg, &gatt);

    /* Notify every 2 s if a client is connected */
    uint16_t conn = 0xFFFF;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        s_counter++;
        if (conn != 0xFFFF) {
            blusys_ble_gatt_notify(gatt, conn, s_val_handle,
                                   &s_counter, sizeof(s_counter));
        }
    }
}
```

## APIs Used

- `blusys_ble_gatt_open()` — registers services, initializes NimBLE, starts advertising; blocks up to 5 s
- `blusys_ble_gatt_notify()` — sends a notification to a subscribed client
- `blusys_ble_gatt_close()` — stops advertising, shuts down the NimBLE stack, frees the handle

## UUID Format

All UUIDs in this module are 128-bit values stored in **little-endian byte order** — index 0 of the `uuid[16]` array is the least significant byte.

If your UUID is `12345678-1234-1234-1234-1234567890ab` (the conventional hyphenated format), convert it to the byte array by reversing each group and then the entire sequence:

```
UUID string: 12345678-1234-1234-1234-1234567890ab
Byte array:  { 0xab, 0x90, 0x78, 0x56, 0x34, 0x12,
               0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
               0x78, 0x56, 0x34, 0x12 }
```

Use an online UUID converter or the Python snippet `bytes.fromhex("123456781234123412341234567890ab")[::-1].hex()` to verify.

## sdkconfig Requirements

BLE and the peripheral role require explicit opt-in. Add a `sdkconfig.defaults` to your project:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y
```

On **ESP32 only**, also set the BT controller mode (ESP32-C3 and ESP32-S3 are BLE-only in hardware):

```
CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y
```

The `examples/ble_gatt_basic/` example provides `sdkconfig.defaults` with these values preset.

## Common Mistakes

- **Forgetting `val_handle_out`** — if a characteristic has `F_NOTIFY` set, `val_handle_out` must point to a valid `uint16_t`. If it is NULL, the characteristic handle is not recorded and `blusys_ble_gatt_notify()` will produce incorrect results.
- **Wrong UUID byte order** — the most common mistake. The `uuid[16]` array is little-endian: `uuid[0]` is the LSB of the last UUID group, not the first. Use the conversion method above.
- **Calling `close()` from a callback** — `read_cb`, `write_cb`, and `conn_cb` run from the NimBLE host task. Calling `blusys_ble_gatt_close()` from within them will deadlock. Signal a FreeRTOS task or event group instead.
- **Using `blusys_bluetooth` and `blusys_ble_gatt` simultaneously** — both modules use the NimBLE singleton stack. Open only one at a time; close the first before opening the second.
- **Notifying before the client subscribes** — the client must enable the CCCD (subscribe) before notifications are delivered. The NimBLE stack silently discards notifications to unsubscribed clients.
- **Device name longer than 29 bytes** — the advertising payload is 31 bytes with 2 bytes used for the AD structure header. Names longer than 29 bytes will be truncated.

## Example App

See `examples/ble_gatt_basic/` for a runnable example that demonstrates a read-notify characteristic (counter) and a read-write characteristic (configuration).

Flash and monitor with:

```
./run.sh examples/ble_gatt_basic <port> esp32
```

Then connect with **nRF Connect** (iOS/Android): scan for `BlusysGATT`, tap Connect, find the custom service, and enable notifications on the first characteristic to see the counter increment every 2 s.

## API Reference

For full type definitions and function signatures, see [BLE GATT API Reference](../modules/ble_gatt.md).
