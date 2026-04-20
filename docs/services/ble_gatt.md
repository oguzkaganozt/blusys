# BLE GATT Server

BLE GATT server for exposing sensor data and accepting commands over Bluetooth Low Energy.

## Quick Example

```c
#include <string.h>
#include "blusys/blusys.h"

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

## UUID Format

All UUIDs in this module are 128-bit values stored in **little-endian byte order** — index 0 of the `uuid[16]` array is the least significant byte.

If your UUID is `12345678-1234-1234-1234-1234567890ab` (the conventional hyphenated format), convert it by reversing the entire byte sequence:

```
UUID string: 12345678-1234-1234-1234-1234567890ab
Byte array:  { 0xab, 0x90, 0x78, 0x56, 0x34, 0x12,
               0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
               0x78, 0x56, 0x34, 0x12 }
```

Use `bytes.fromhex("123456781234123412341234567890ab")[::-1].hex()` in Python to verify.

## Common Mistakes

- **forgetting `val_handle_out`** — if a characteristic has `F_NOTIFY` set, `val_handle_out` must point to a valid `uint16_t`. Otherwise `blusys_ble_gatt_open()` returns `BLUSYS_ERR_INVALID_ARG`.
- **wrong UUID byte order** — the most common mistake. The `uuid[16]` array is little-endian: `uuid[0]` is the LSB of the last UUID group, not the first.
- **calling `close()` from a callback** — `read_cb`, `write_cb`, and `conn_cb` run from the NimBLE host task. Calling `blusys_ble_gatt_close()` from within them deadlocks. Signal a FreeRTOS task or event group instead.
- **notifying before the client subscribes** — the client must enable the CCCD (subscribe) before notifications are delivered. The NimBLE stack silently discards notifications to unsubscribed clients.
- **device name longer than 29 bytes** — `blusys_ble_gatt_open()` returns `BLUSYS_ERR_INVALID_ARG`.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! warning
    `blusys_ble_gatt` owns the BLE host stack while open. It cannot be open at the same time as `blusys_bluetooth`, BLE-transport `blusys_usb_hid`, BLE-transport `blusys_wifi_prov`, or `blusys_ble_hid_device`.

## Thread Safety

All public functions are thread-safe. The `read_cb`, `write_cb`, and `conn_cb` callbacks are invoked from the NimBLE host task — do not call `blusys_ble_gatt_close()` from within any callback.

## Stack Notes

NimBLE and the peripheral role must be enabled in sdkconfig:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y
```

The `examples/validation/wireless_bt_lab` example (BLE GATT scenario) provides `sdkconfig.defaults` with these values preset, and `sdkconfig.esp32` also selects BLE-only controller mode on ESP32.

## Limitations

- GATT server (peripheral) role only — no GATT client
- 128-bit UUIDs only — 16-bit Bluetooth SIG UUIDs are not directly supported
- Single active connection tracked for notifications; multi-client notify requires iterating connections at the application level
- Maximum read/write payload handled by the internal buffer: 512 bytes
- One active BLE-owning service at a time

## Example App

See `examples/validation/wireless_bt_lab` (BLE GATT scenario).
