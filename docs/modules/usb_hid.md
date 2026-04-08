# USB HID

HID input service with dual-transport support: USB OTG host (ESP32-S3, via `espressif/usb_host_hid`) and BLE HOGP central (all targets, via NimBLE). Parses boot-protocol keyboard and mouse reports and delivers them through a unified callback.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [USB HID Basics](../guides/usb-hid-basic.md).

## Target Support

| Target | USB transport | BLE transport |
|--------|--------------|---------------|
| ESP32 | no | yes (requires NimBLE) |
| ESP32-C3 | no | yes (requires NimBLE) |
| ESP32-S3 | yes (requires `espressif/usb_host_hid`) | yes (requires NimBLE) |

`BLUSYS_FEATURE_USB_HID` is set on all targets. The USB transport additionally requires `BLUSYS_FEATURE_USB_HOST`. Requesting an unsupported transport returns `BLUSYS_ERR_NOT_SUPPORTED`.

!!! note "Managed component — USB transport"
    Add `espressif/usb_host_hid: "~1.1.0"` to your project's `main/idf_component.yml`. See `examples/usb_hid_basic/main/idf_component.yml`.

!!! note "BLE transport"
    Enable `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_CENTRAL=y`, and `CONFIG_BT_NIMBLE_ROLE_OBSERVER=y`. Requires NVS (initialized automatically).

## Types

### `blusys_usb_hid_t`

```c
typedef struct blusys_usb_hid blusys_usb_hid_t;
```

Opaque handle returned by `blusys_usb_hid_open()`.

### `blusys_usb_hid_transport_t`

```c
typedef enum {
    BLUSYS_USB_HID_TRANSPORT_USB,
    BLUSYS_USB_HID_TRANSPORT_BLE,
} blusys_usb_hid_transport_t;
```

### `blusys_usb_hid_keyboard_report_t`

```c
typedef struct {
    uint8_t modifier;
    uint8_t keycodes[6];
} blusys_usb_hid_keyboard_report_t;
```

HID boot-protocol keyboard report. `modifier` is a bitmask of modifier keys (Ctrl, Shift, Alt, GUI). `keycodes` contains up to 6 simultaneously pressed USB HID key codes.

### `blusys_usb_hid_mouse_report_t`

```c
typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} blusys_usb_hid_mouse_report_t;
```

HID boot-protocol mouse report. `x`, `y`, and `wheel` are signed relative movements.

### `blusys_usb_hid_raw_report_t`

```c
typedef struct {
    const uint8_t *data;
    size_t         len;
} blusys_usb_hid_raw_report_t;
```

Raw HID report for devices that do not fit the boot-protocol keyboard or mouse layout (e.g. gamepads). The `data` pointer is valid only for the duration of the callback.

### `blusys_usb_hid_event_t`

```c
typedef enum {
    BLUSYS_USB_HID_EVENT_CONNECTED,
    BLUSYS_USB_HID_EVENT_DISCONNECTED,
    BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT,
    BLUSYS_USB_HID_EVENT_MOUSE_REPORT,
    BLUSYS_USB_HID_EVENT_RAW_REPORT,
} blusys_usb_hid_event_t;
```

### `blusys_usb_hid_event_data_t`

```c
typedef struct {
    blusys_usb_hid_event_t event;
    union {
        blusys_usb_hid_keyboard_report_t keyboard;
        blusys_usb_hid_mouse_report_t    mouse;
        blusys_usb_hid_raw_report_t      raw;
    } data;
} blusys_usb_hid_event_data_t;
```

Access `data.keyboard`, `data.mouse`, or `data.raw` based on the `event` field.

### `blusys_usb_hid_config_t`

```c
typedef struct {
    blusys_usb_hid_transport_t transport;
    blusys_usb_hid_callback_t  callback;
    void                      *user_ctx;
    const char                *ble_device_name;
} blusys_usb_hid_config_t;
```

`ble_device_name` — BLE transport only. If non-NULL, only connect to a peripheral whose advertised name exactly matches. If NULL, the first HID peripheral found is used.

## Functions

### `blusys_usb_hid_open`

```c
blusys_err_t blusys_usb_hid_open(const blusys_usb_hid_config_t *config,
                                  blusys_usb_hid_t **out_hid);
```

Opens the HID service on the requested transport. For the BLE transport, starts NimBLE and begins scanning for HID peripherals. For the USB transport, installs the USB host stack and the HID class driver.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE` if already open, `BLUSYS_ERR_NOT_SUPPORTED` if the transport is unavailable, `BLUSYS_ERR_TIMEOUT` if BLE sync fails.

---

### `blusys_usb_hid_close`

```c
blusys_err_t blusys_usb_hid_close(blusys_usb_hid_t *hid);
```

Terminates the transport (disconnects BLE or uninstalls USB host/HID driver) and frees the handle.

## Lifecycle

**USB transport:**

1. `blusys_usb_hid_open()` — install USB host lib + HID class driver
2. `callback` fires with `BLUSYS_USB_HID_EVENT_CONNECTED` when a HID device is attached
3. Reports arrive as `KEYBOARD_REPORT`, `MOUSE_REPORT`, or `RAW_REPORT` events
4. `blusys_usb_hid_close()` — uninstall

**BLE transport:**

1. `blusys_usb_hid_open()` — init NimBLE, start scanning
2. Automatically connects to the first matching HID peripheral
3. `callback` fires with `BLUSYS_USB_HID_EVENT_CONNECTED` after HOGP subscription is complete
4. Reports arrive via GATT notifications, parsed into keyboard/mouse/raw events
5. On disconnect, scanning resumes automatically
6. `blusys_usb_hid_close()` — stop NimBLE

## Thread Safety

- Only one instance is allowed at a time; a second `open()` returns `BLUSYS_ERR_INVALID_STATE`
- The callback is invoked from the USB HID task (USB transport) or NimBLE host task (BLE transport) — do not call `blusys_usb_hid_close()` from within it

## Limitations

- Raw report `data` pointer is valid only during the callback — copy if needed
- Boot-protocol parsing heuristic: 8-byte reports are treated as keyboards, 3–4-byte reports as mice, shorter reports as raw
- BLE transport initiates a new scan after each disconnect; `ble_device_name` filter applies to every reconnect attempt
- BLE transport cannot be used simultaneously with `blusys_bluetooth`, `blusys_ble_gatt`, or BLE transport `blusys_wifi_prov`

## Example App

See `examples/usb_hid_basic/`.
