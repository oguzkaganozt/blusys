# USB HID

HID input service for USB OTG host or BLE HOGP central.

## At a glance

- one instance at a time
- USB host transport is ESP32-S3 only
- BLE transport is available on all targets with NimBLE

## Quick example

```c
static void on_hid(blusys_usb_hid_t *hid,
                   const blusys_usb_hid_event_data_t *event,
                   void *ctx)
{
    (void)hid; (void)ctx;
    if (event->event == BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT) {
        printf("keyboard report\n");
    }
}

blusys_usb_hid_t *hid;
blusys_usb_hid_config_t cfg = {
    .transport = BLUSYS_USB_HID_TRANSPORT_USB,
    .callback = on_hid,
};
blusys_usb_hid_open(&cfg, &hid);
```

## Common mistakes

- requesting USB transport on a non-S3 target
- enabling BLE transport without the required NimBLE roles
- mixing BLE HID with other BLE-owning services
- keeping the report pointer after the callback returns

## Target support

| Target | USB transport | BLE transport |
|--------|---------------|---------------|
| ESP32 | no | yes |
| ESP32-C3 | no | yes |
| ESP32-S3 | yes | yes |

## Thread safety

- only one instance is allowed at a time
- callbacks run on the transport task, not in ISR context

## Limitations

- raw report data is borrowed for the callback duration only
- BLE transport cannot coexist with other BLE-owning services

## Example app

`examples/validation/usb_peripheral_lab`
