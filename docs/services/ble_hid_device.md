# BLE HID Device (HOGP)

Standards-compliant BLE HID peripheral (HID over GATT Profile) for building media remotes, keyboards, and gamepads that any modern OS pairs with natively — no companion app required.

## Quick Example

```c
#include "blusys/services/connectivity/ble_hid_device.h"

static blusys_ble_hid_device_t *s_hid;

static void on_conn(bool connected, void *user_ctx)
{
    printf("HID %s\n", connected ? "connected" : "advertising");
}

void app_main(void)
{
    blusys_ble_hid_device_config_t cfg = {
        .device_name         = "MyRemote",
        .vendor_id           = 0x303A,
        .product_id          = 0x8001,
        .version             = 0x0100,
        .initial_battery_pct = 100,
        .conn_cb             = on_conn,
    };
    blusys_ble_hid_device_open(&cfg, &s_hid);

    // Press-and-release Volume Up every second.
    while (1) {
        if (blusys_ble_hid_device_is_connected(s_hid)) {
            blusys_ble_hid_device_send_consumer(s_hid, BLUSYS_HID_USAGE_VOLUME_UP, true);
            vTaskDelay(pdMS_TO_TICKS(50));
            blusys_ble_hid_device_send_consumer(s_hid, BLUSYS_HID_USAGE_VOLUME_UP, false);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

For a product-level wiring, see the `bluetooth_controller` quickstart — it uses the `blusys::ble_hid_device_capability` wrapper and three GPIO buttons.

## Service surface

On open, three SIG-standard services are registered:

| Service | UUID | Characteristics |
|---|---|---|
| HID | `0x1812` | Report Map `0x2A4B` (read-enc), HID Information `0x2A4A`, HID Control Point `0x2A4C`, Input Report `0x2A4D` (read-enc, notify) + Report Reference `0x2908` + CCCD `0x2902` |
| Battery | `0x180F` | Battery Level `0x2A19` (read, notify) |
| Device Info | `0x180A` | PnP ID `0x2A50` |

Advertising payload carries flags, appearance (`0x03C1` — HID Generic) and the HID service UUID; the scan response carries the complete local name.

## Report map

The built-in report map declares a **Consumer Control** application collection with one 1-byte input report:

| Bit | HID Usage | Meaning |
|-----|-----------|---------|
| 0 | `0x00E9` | Volume Increment |
| 1 | `0x00EA` | Volume Decrement |
| 2 | `0x00E2` | Mute |
| 3 | `0x00CD` | Play/Pause |
| 4 | `0x00B5` | Next Track |
| 5 | `0x00B6` | Previous Track |
| 6 | `0x006F` | Brightness Up |
| 7 | `0x0070` | Brightness Down |

Use `blusys_ble_hid_device_send_consumer()` for edge-triggered updates. For a different report map (full keyboard, mouse, gamepad), use `blusys_ble_hid_device_send_report()` as an escape hatch — you own the payload bytes and the service keeps sending them on the Input Report characteristic.

## Pairing

Just-Works bonding (no passkey), persisted in NVS via the NimBLE default store. After the first pair, the device reconnects automatically on subsequent power cycles. Expose a factory-reset path if end-users need to re-pair with a new host.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note
    BLE only, via the NimBLE stack. Enable `CONFIG_BT_ENABLED` and `CONFIG_BT_NIMBLE_ENABLED` in sdkconfig. Peripheral role is sufficient — central is not needed.

!!! warning
    `blusys_ble_hid_device` owns the BLE host stack while open. It cannot coexist with `blusys_bluetooth`, `blusys_ble_gatt`, BLE-transport `blusys_usb_hid`, or BLE-transport `blusys_wifi_prov`. The second opener receives `BLUSYS_ERR_BUSY`.

## Thread Safety

Open / close from a single task. `send_consumer`, `send_report`, `set_battery`, and `is_connected` are safe to call from any task; they serialize through an internal lock.

## Stack Notes

Uses NimBLE. If you also need Wi-Fi provisioning, provision before enabling HID (or tear the HID capability down first) — the BLE controller is single-owner.

## Limitations

- only one `blusys_ble_hid_device` instance may be open at a time
- cannot be used alongside `blusys_bluetooth`, `blusys_ble_gatt`, BLE-transport `blusys_usb_hid`, or BLE-transport `blusys_wifi_prov` — the second opener receives `BLUSYS_ERR_BUSY`
- built-in report map is Consumer Control only (one 1-byte input report); full keyboard/mouse/gamepad reports require `blusys_ble_hid_device_send_report()` with a caller-supplied payload
- pairing is Just-Works only — no passkey or out-of-band bonding
- bond storage uses the NimBLE default NVS store; expose a factory-reset path if end-users need to re-pair with a new host

## Example App

See `examples/quickstart/bluetooth_controller` — wires `blusys::ble_hid_device_capability` to three GPIO buttons for a minimal media remote.
