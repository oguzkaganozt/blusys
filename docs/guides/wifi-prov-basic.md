# Provision WiFi Credentials at Runtime

## Problem Statement

You want to ship a device without hard-coded WiFi credentials. At first boot the device advertises a provisioning service via BLE or SoftAP so the user can send SSID and password from a mobile app. Credentials are stored to NVS and the provisioning flow is skipped on subsequent boots.

## Prerequisites

- A supported board (ESP32, ESP32-C3, or ESP32-S3)
- [ESP BLE Provisioning](https://apps.apple.com/app/esp-ble-provisioning/id1473590141) or [ESP SoftAP Provisioning](https://apps.apple.com/app/esp-softap-provisioning/id1474040630) mobile app
- For BLE transport: `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y`

## Minimal Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "blusys/blusys_all.h"

#define PROV_DONE_BIT BIT0

static EventGroupHandle_t s_done;

static void on_event(blusys_wifi_prov_event_t event,
                     const blusys_wifi_prov_credentials_t *creds,
                     void *ctx)
{
    if (event == BLUSYS_WIFI_PROV_EVENT_SUCCESS ||
        event == BLUSYS_WIFI_PROV_EVENT_FAILED) {
        xEventGroupSetBits(s_done, PROV_DONE_BIT);
    }
}

void app_main(void)
{
    s_done = xEventGroupCreate();

    if (blusys_wifi_prov_is_provisioned()) {
        printf("Already provisioned.\n");
        return;
    }

    blusys_wifi_prov_t *prov;
    blusys_wifi_prov_config_t cfg = {
        .transport    = BLUSYS_WIFI_PROV_TRANSPORT_BLE,
        .service_name = "PROV_DEVICE",
        .pop          = "abcd1234",
        .on_event     = on_event,
    };
    blusys_wifi_prov_open(&cfg, &prov);

    char qr[256];
    blusys_wifi_prov_get_qr_payload(prov, qr, sizeof(qr));
    printf("QR payload: %s\n", qr);

    blusys_wifi_prov_start(prov);

    xEventGroupWaitBits(s_done, PROV_DONE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    blusys_wifi_prov_stop(prov);
    blusys_wifi_prov_close(prov);
}
```

## APIs Used

- `blusys_wifi_prov_is_provisioned()` — checks NVS for stored credentials; no handle required
- `blusys_wifi_prov_open()` — initializes the WiFi stack and provisioning manager
- `blusys_wifi_prov_get_qr_payload()` — writes the JSON payload for the mobile app QR scanner
- `blusys_wifi_prov_start()` — starts advertising / starts the SoftAP
- `blusys_wifi_prov_stop()` — stops the service
- `blusys_wifi_prov_close()` — tears down the stack and frees the handle
- `blusys_wifi_prov_reset()` — erases stored credentials from NVS

## Provisioning Flow

1. At boot, call `blusys_wifi_prov_is_provisioned()`. If `true`, load credentials from NVS via `blusys_wifi_open()` / `blusys_wifi_connect()` and skip provisioning.
2. If not provisioned, open the provisioning service and start it.
3. Print or display the QR payload — paste it into [qr.io](https://qr.io) or use the "Scan QR code" feature in the ESP Provisioning app.
4. The user opens the app, scans the QR code, selects their network, and enters the password.
5. The device receives credentials (`CREDENTIALS_RECEIVED` event) and attempts to connect.
6. On success (`SUCCESS` event), credentials are persisted to NVS automatically by ESP-IDF.
7. Stop and close the provisioning service.
8. On next boot, `is_provisioned()` returns `true` and provisioning is skipped.

## Transport Selection

### BLE

- Device advertises itself as a BLE peripheral.
- Use the **ESP BLE Provisioning** app.
- Requires `CONFIG_BT_NIMBLE_ENABLED` — enable in menuconfig.
- BLE stack is freed after provisioning completes (`WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM`).

### SoftAP

- Device creates a temporary WiFi access point.
- Connect your phone to that AP, then use the **ESP SoftAP Provisioning** app.
- No BLE dependency; works on all targets unconditionally.
- Set `service_key` in the config to password-protect the AP.

## Proof of Possession (PoP)

The `pop` field adds a security layer — the mobile app prompts the user to enter this string, preventing unauthorized provisioning.

```c
blusys_wifi_prov_config_t cfg = {
    .transport    = BLUSYS_WIFI_PROV_TRANSPORT_BLE,
    .service_name = "PROV_DEVICE",
    .pop          = "abcd1234",   /* user must enter this in the app */
    .on_event     = on_event,
};
```

Pass `NULL` to disable PoP (uses `WIFI_PROV_SECURITY_0`).

## Resetting Credentials

To force re-provisioning, erase the stored credentials:

```c
blusys_wifi_prov_reset();
/* blusys_wifi_prov_is_provisioned() now returns false */
```

## Expected Runtime Behavior

- `open()` initializes the WiFi stack and provisioning manager.
- `start()` begins advertising; `STARTED` event fires when the service is ready.
- `CREDENTIALS_RECEIVED` fires immediately after the app sends the SSID and password.
- ESP-IDF's provisioning manager validates the credentials by attempting a WiFi connection.
- `SUCCESS` fires after the connection is confirmed and the IP assigned; credentials are saved to NVS.
- `FAILED` fires if the connection attempt times out or authentication fails; the app can retry.

## Common Mistakes

- Calling `blusys_wifi_prov_is_provisioned()` before `open()` returns `false` because the provisioning manager is not initialized yet — call it after `open()`.
- Forgetting the BLE transport requirements (`CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y`) — `open()` returns `BLUSYS_ERR_NOT_SUPPORTED`.
- Opening BLE provisioning while `blusys_bluetooth`, `blusys_ble_gatt`, or BLE `blusys_usb_hid` is already open — `open()` returns `BLUSYS_ERR_INVALID_STATE`.
- Using a 5 GHz SSID — the ESP32 radio supports 2.4 GHz only.
- Not calling `close()` after provisioning — the WiFi stack and NVS partition are not released.

## Example App

See `examples/wifi_prov_basic/` for a runnable example with both BLE and SoftAP transport selectable via menuconfig.

## API Reference

For full type definitions and function signatures, see [WiFi Provisioning API Reference](../modules/wifi_prov.md).
