/**
 * @file usb_host.h
 * @brief USB host stack lifecycle and device enumeration events.
 *
 * First release exposes only host initialisation and connect/disconnect
 * callbacks; per-class operations (HID, MSC, CDC host) are expected to grow.
 * See docs/hal/usb_host.md.
 */

#ifndef BLUSYS_USB_HOST_H
#define BLUSYS_USB_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque USB host handle. */
typedef struct blusys_usb_host blusys_usb_host_t;

/** @brief Metadata reported for a connected USB device. */
typedef struct {
    uint16_t vid;       /**< USB Vendor ID. */
    uint16_t pid;       /**< USB Product ID. */
    uint8_t  dev_addr;  /**< Bus-assigned device address. */
} blusys_usb_host_device_info_t;

/** @brief USB host event kind. */
typedef enum {
    BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED,      /**< A device was enumerated. */
    BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED,   /**< A device was unplugged. */
} blusys_usb_host_event_t;

/**
 * @brief Callback fired from a host-task context for enumeration events.
 * @param host     Host that raised the event.
 * @param event    Event kind.
 * @param device   Device metadata (valid for the duration of the call).
 * @param user_ctx User pointer supplied via ::blusys_usb_host_config_t.
 */
typedef void (*blusys_usb_host_event_callback_t)(
    blusys_usb_host_t *host,
    blusys_usb_host_event_t event,
    const blusys_usb_host_device_info_t *device,
    void *user_ctx);

/** @brief Configuration for ::blusys_usb_host_open. */
typedef struct {
    blusys_usb_host_event_callback_t event_cb;       /**< Event callback; may be NULL. */
    void                            *event_user_ctx; /**< Pointer forwarded to @p event_cb. */
} blusys_usb_host_config_t;

/**
 * @brief Install and start the USB host stack.
 * @param config   Configuration, must be non-NULL.
 * @param out_host Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without a USB host controller.
 */
blusys_err_t blusys_usb_host_open(const blusys_usb_host_config_t *config,
                                   blusys_usb_host_t **out_host);

/**
 * @brief Stop the USB host stack and free the handle.
 * @param host  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p host is NULL.
 */
blusys_err_t blusys_usb_host_close(blusys_usb_host_t *host);

#ifdef __cplusplus
}
#endif

#endif
