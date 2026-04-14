#ifndef BLUSYS_USB_DEVICE_H
#define BLUSYS_USB_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_usb_device blusys_usb_device_t;

typedef enum {
    BLUSYS_USB_DEVICE_CLASS_CDC,
    BLUSYS_USB_DEVICE_CLASS_HID,
    BLUSYS_USB_DEVICE_CLASS_MSC,
} blusys_usb_device_class_t;

typedef enum {
    BLUSYS_USB_DEVICE_EVENT_CONNECTED,
    BLUSYS_USB_DEVICE_EVENT_DISCONNECTED,
    BLUSYS_USB_DEVICE_EVENT_SUSPENDED,
    BLUSYS_USB_DEVICE_EVENT_RESUMED,
} blusys_usb_device_event_t;

typedef void (*blusys_usb_device_event_callback_t)(
    blusys_usb_device_t *dev,
    blusys_usb_device_event_t event,
    void *user_ctx);

typedef void (*blusys_usb_device_cdc_rx_callback_t)(
    blusys_usb_device_t *dev,
    const uint8_t *data,
    size_t len,
    void *user_ctx);

typedef struct {
    uint16_t    vid;
    uint16_t    pid;
    const char *manufacturer;
    const char *product;
    const char *serial;
    blusys_usb_device_class_t device_class;
    blusys_usb_device_event_callback_t event_cb;
    void *event_user_ctx;
} blusys_usb_device_config_t;

blusys_err_t blusys_usb_device_open(const blusys_usb_device_config_t *config,
                                     blusys_usb_device_t **out_dev);
blusys_err_t blusys_usb_device_close(blusys_usb_device_t *dev);

/* CDC class operations */
blusys_err_t blusys_usb_device_cdc_write(blusys_usb_device_t *dev,
                                          const void *data, size_t len);
blusys_err_t blusys_usb_device_cdc_set_rx_callback(
    blusys_usb_device_t *dev,
    blusys_usb_device_cdc_rx_callback_t callback,
    void *user_ctx);

/* HID class operations */
blusys_err_t blusys_usb_device_hid_send_report(blusys_usb_device_t *dev,
                                                 const uint8_t *report,
                                                 size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_DEVICE_H */
