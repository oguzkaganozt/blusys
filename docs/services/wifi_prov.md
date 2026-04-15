# WiFi Provisioning

BLE or SoftAP-based runtime WiFi credential provisioning with NVS persistence.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

BLE transport requires `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y` in the project sdkconfig. SoftAP transport works on all targets unconditionally.

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

## Types

### `blusys_wifi_prov_transport_t`

```c
typedef enum {
    BLUSYS_WIFI_PROV_TRANSPORT_BLE,     /* BLE transport — requires CONFIG_BT_NIMBLE_ENABLED */
    BLUSYS_WIFI_PROV_TRANSPORT_SOFTAP,  /* SoftAP transport — no BLE dependency */
} blusys_wifi_prov_transport_t;
```

### `blusys_wifi_prov_event_t`

```c
typedef enum {
    BLUSYS_WIFI_PROV_EVENT_STARTED,               /* service is advertising / AP is up */
    BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED,  /* SSID + password delivered by client */
    BLUSYS_WIFI_PROV_EVENT_SUCCESS,               /* credentials validated — device connected */
    BLUSYS_WIFI_PROV_EVENT_FAILED,                /* credential validation failed */
} blusys_wifi_prov_event_t;
```

### `blusys_wifi_prov_credentials_t`

```c
typedef struct {
    char ssid[33];      /* SSID of the target AP */
    char password[65];  /* password; empty string for open networks */
} blusys_wifi_prov_credentials_t;
```

Only populated when the event is `BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED`.

### `blusys_wifi_prov_cb_t`

```c
typedef void (*blusys_wifi_prov_cb_t)(
    blusys_wifi_prov_event_t event,
    const blusys_wifi_prov_credentials_t *creds,
    void *user_ctx
);
```

### `blusys_wifi_prov_config_t`

```c
typedef struct {
    blusys_wifi_prov_transport_t transport;  /* BLE or SoftAP */
    const char *service_name;               /* BLE device name or SoftAP SSID (required) */
    const char *pop;                        /* proof-of-possession string; NULL disables security */
    const char *service_key;                /* SoftAP password; NULL = open AP; ignored for BLE */
    blusys_wifi_prov_cb_t on_event;         /* event callback (required) */
    void *user_ctx;                         /* passed back in every on_event call */
} blusys_wifi_prov_config_t;
```

## Functions

### `blusys_wifi_prov_open`

```c
blusys_err_t blusys_wifi_prov_open(const blusys_wifi_prov_config_t *config,
                                   blusys_wifi_prov_t **out_prov);
```

Initializes the WiFi stack and provisioning manager. Only one instance may be open at a time.

Calls `nvs_flash_init()`, `esp_netif_init()`, and `esp_event_loop_create_default()` internally. Do not call them separately in the same application.

**Parameters:**
- `config` — service configuration (required); `service_name` and `on_event` must be non-NULL
- `out_prov` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if an instance is already open or another BLE-owning module already holds the BLE stack, `BLUSYS_ERR_NOT_SUPPORTED` if BLE transport is requested but `CONFIG_BT_NIMBLE_ENABLED` is not set, `BLUSYS_ERR_NO_MEM` on allocation failure.

---

### `blusys_wifi_prov_close`

```c
blusys_err_t blusys_wifi_prov_close(blusys_wifi_prov_t *prov);
```

Stops provisioning if running, deinitializes the manager and WiFi stack, and frees the handle. After this call the handle is invalid.

---

### `blusys_wifi_prov_start`

```c
blusys_err_t blusys_wifi_prov_start(blusys_wifi_prov_t *prov);
```

Starts advertising the provisioning service. For BLE transport, begins BLE advertising. For SoftAP transport, starts the access point and HTTP server.

The `on_event` callback fires `BLUSYS_WIFI_PROV_EVENT_STARTED` when the service is ready.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if already running.

---

### `blusys_wifi_prov_stop`

```c
blusys_err_t blusys_wifi_prov_stop(blusys_wifi_prov_t *prov);
```

Stops the provisioning service advertisement. Safe to call even if not running.

---

### `blusys_wifi_prov_get_qr_payload`

```c
blusys_err_t blusys_wifi_prov_get_qr_payload(blusys_wifi_prov_t *prov,
                                              char *buf, size_t buf_size);
```

Writes the JSON payload expected by the ESP BLE Provisioning and ESP SoftAP Provisioning mobile apps when scanning a QR code.

Example output:
```
{"ver":"v1","name":"PROV_DEVICE","pop":"abcd1234","transport":"ble"}
```

**Parameters:**
- `buf` — output buffer; 256 bytes is sufficient
- `buf_size` — size of `buf`

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `buf` is NULL or `buf_size` is 0.

---

### `blusys_wifi_prov_is_provisioned`

```c
bool blusys_wifi_prov_is_provisioned(void);
```

Queries the ESP-IDF provisioning manager for stored credentials. Call this after `blusys_wifi_prov_open()` or after your application has initialized the provisioning manager separately.

**Returns:** `true` if credentials are stored, `false` otherwise.

---

### `blusys_wifi_prov_reset`

```c
blusys_err_t blusys_wifi_prov_reset(void);
```

Clears stored WiFi provisioning credentials via the ESP-IDF provisioning manager. After the manager is initialized again, `blusys_wifi_prov_is_provisioned()` returns `false`.

**Returns:** `BLUSYS_OK` on success, or a translated ESP-IDF error on failure.

## Lifecycle

```
blusys_wifi_prov_open()
    └─ blusys_wifi_prov_start()
           │  (STARTED event fires)
           │  (user scans QR code, enters credentials in app)
           │  (CREDENTIALS_RECEIVED event fires)
           │  (SUCCESS or FAILED event fires)
           └─ blusys_wifi_prov_stop()
blusys_wifi_prov_close()
```

## Thread Safety

All handle-based functions are protected by a FreeRTOS mutex. `blusys_wifi_prov_is_provisioned()` and `blusys_wifi_prov_reset()` are stateless helpers around the ESP-IDF provisioning manager and are safe to call from any task once the manager has been initialized.

## Limitations

- Only one provisioning instance may be open at a time.
- BLE transport requires `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y`, and `CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y`.
- BLE transport cannot be used simultaneously with `blusys_bluetooth`, `blusys_ble_gatt`, or BLE `blusys_usb_hid`.
- 2.4 GHz networks only.

## Example App

See `examples/validation/network_services/` (WiFi provisioning scenario).
