#ifndef BLUSYS_USB_HOST_H
#define BLUSYS_USB_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_usb_host blusys_usb_host_t;

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint8_t  dev_addr;
} blusys_usb_host_device_info_t;

typedef enum {
    BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED,
    BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED,
} blusys_usb_host_event_t;

typedef void (*blusys_usb_host_event_callback_t)(
    blusys_usb_host_t *host,
    blusys_usb_host_event_t event,
    const blusys_usb_host_device_info_t *device,
    void *user_ctx);

typedef struct {
    blusys_usb_host_event_callback_t event_cb;
    void *event_user_ctx;
} blusys_usb_host_config_t;

blusys_err_t blusys_usb_host_open(const blusys_usb_host_config_t *config,
                                   blusys_usb_host_t **out_host);
blusys_err_t blusys_usb_host_close(blusys_usb_host_t *host);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_HOST_H */
