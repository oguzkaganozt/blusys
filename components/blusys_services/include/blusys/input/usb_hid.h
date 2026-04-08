#ifndef BLUSYS_USB_HID_H
#define BLUSYS_USB_HID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_usb_hid blusys_usb_hid_t;

typedef enum {
    BLUSYS_USB_HID_TRANSPORT_USB,   /* ESP32-S3 only (USB OTG host + usb_host_hid driver) */
    BLUSYS_USB_HID_TRANSPORT_BLE,   /* All targets (NimBLE HOGP client) */
} blusys_usb_hid_transport_t;

typedef struct {
    uint8_t modifier;
    uint8_t keycodes[6];
} blusys_usb_hid_keyboard_report_t;

typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} blusys_usb_hid_mouse_report_t;

typedef struct {
    const uint8_t *data;
    size_t         len;
} blusys_usb_hid_raw_report_t;

typedef enum {
    BLUSYS_USB_HID_EVENT_CONNECTED,
    BLUSYS_USB_HID_EVENT_DISCONNECTED,
    BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT,
    BLUSYS_USB_HID_EVENT_MOUSE_REPORT,
    BLUSYS_USB_HID_EVENT_RAW_REPORT,
} blusys_usb_hid_event_t;

typedef struct {
    blusys_usb_hid_event_t event;
    union {
        blusys_usb_hid_keyboard_report_t keyboard;
        blusys_usb_hid_mouse_report_t    mouse;
        blusys_usb_hid_raw_report_t      raw;
    } data;
} blusys_usb_hid_event_data_t;

typedef void (*blusys_usb_hid_callback_t)(
    blusys_usb_hid_t *hid,
    const blusys_usb_hid_event_data_t *event,
    void *user_ctx);

typedef struct {
    blusys_usb_hid_transport_t transport;
    blusys_usb_hid_callback_t  callback;
    void                      *user_ctx;
    const char                *ble_device_name; /* BLE only: filter by peripheral name (NULL = any HID device) */
} blusys_usb_hid_config_t;

blusys_err_t blusys_usb_hid_open(const blusys_usb_hid_config_t *config,
                                  blusys_usb_hid_t **out_hid);
blusys_err_t blusys_usb_hid_close(blusys_usb_hid_t *hid);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_HID_H */
