# WiFi Provisioning

BLE or SoftAP-based runtime WiFi credential provisioning with NVS persistence.

## Quick Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "blusys/blusys.h"

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

    blusys_wifi_prov_t *prov;
    blusys_wifi_prov_config_t cfg = {
        .transport    = BLUSYS_WIFI_PROV_TRANSPORT_BLE,
        .service_name = "PROV_DEVICE",
        .pop          = "abcd1234",
        .on_event     = on_event,
    };
    blusys_wifi_prov_open(&cfg, &prov);

    /* is_provisioned() requires the manager to be initialized — call after open() */
    if (blusys_wifi_prov_is_provisioned()) {
        printf("Already provisioned.\n");
        blusys_wifi_prov_close(prov);
        return;
    }

    char qr[256];
    blusys_wifi_prov_get_qr_payload(prov, qr, sizeof(qr));
    printf("QR payload: %s\n", qr);

    blusys_wifi_prov_start(prov);

    xEventGroupWaitBits(s_done, PROV_DONE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    blusys_wifi_prov_stop(prov);
    blusys_wifi_prov_close(prov);
}
```

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

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

BLE transport requires `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y` in the project sdkconfig. SoftAP transport works on all targets unconditionally.

## Thread Safety

All handle-based functions are protected by a FreeRTOS mutex. `blusys_wifi_prov_is_provisioned()` and `blusys_wifi_prov_reset()` are stateless helpers around the ESP-IDF provisioning manager and are safe to call from any task once the manager has been initialized.

## Limitations

- only one provisioning instance may be open at a time.
- BLE transport requires `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y`.
- BLE transport cannot be used simultaneously with `blusys_bluetooth`, `blusys_ble_gatt`, `blusys_ble_hid_device`, or BLE `blusys_usb_hid`.
- 2.4 GHz networks only.

## Example App

See `examples/validation/network_services/` (WiFi provisioning scenario).
