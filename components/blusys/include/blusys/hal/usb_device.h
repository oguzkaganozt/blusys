/**
 * @file usb_device.h
 * @brief USB device stack: CDC serial, HID reports, and MSC enumeration.
 *
 * First release exposes CDC RX/TX and HID send_report; MSC is enumerated
 * but exposes no additional entry points. See docs/hal/usb_device.md.
 */

#ifndef BLUSYS_USB_DEVICE_H
#define BLUSYS_USB_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque USB device handle. */
typedef struct blusys_usb_device blusys_usb_device_t;

/** @brief USB device class supported by ::blusys_usb_device_open. */
typedef enum {
    BLUSYS_USB_DEVICE_CLASS_CDC,    /**< Communications Device Class (virtual serial). */
    BLUSYS_USB_DEVICE_CLASS_HID,    /**< Human Interface Device. */
    BLUSYS_USB_DEVICE_CLASS_MSC,    /**< Mass Storage Class. */
} blusys_usb_device_class_t;

/** @brief USB device bus-state event. */
typedef enum {
    BLUSYS_USB_DEVICE_EVENT_CONNECTED,      /**< Host enumerated the device. */
    BLUSYS_USB_DEVICE_EVENT_DISCONNECTED,   /**< Host disconnected. */
    BLUSYS_USB_DEVICE_EVENT_SUSPENDED,      /**< Bus suspend. */
    BLUSYS_USB_DEVICE_EVENT_RESUMED,        /**< Bus resume. */
} blusys_usb_device_event_t;

/**
 * @brief Bus-state event callback.
 * @param dev       Device that raised the event.
 * @param event     Event kind.
 * @param user_ctx  User pointer supplied via ::blusys_usb_device_config_t.
 */
typedef void (*blusys_usb_device_event_callback_t)(
    blusys_usb_device_t *dev,
    blusys_usb_device_event_t event,
    void *user_ctx);

/**
 * @brief CDC receive callback.
 * @param dev       Device that received the data.
 * @param data      Received bytes (valid for the duration of the call).
 * @param len       Number of bytes in @p data.
 * @param user_ctx  User pointer supplied via ::blusys_usb_device_cdc_set_rx_callback.
 */
typedef void (*blusys_usb_device_cdc_rx_callback_t)(
    blusys_usb_device_t *dev,
    const uint8_t *data,
    size_t len,
    void *user_ctx);

/** @brief Configuration for ::blusys_usb_device_open. */
typedef struct {
    uint16_t                           vid;             /**< USB Vendor ID. */
    uint16_t                           pid;             /**< USB Product ID. */
    const char                        *manufacturer;    /**< Manufacturer string descriptor. */
    const char                        *product;         /**< Product string descriptor. */
    const char                        *serial;          /**< Serial-number string descriptor. */
    blusys_usb_device_class_t          device_class;    /**< Class to enumerate as. */
    blusys_usb_device_event_callback_t event_cb;        /**< Bus-state callback; may be NULL. */
    void                              *event_user_ctx;  /**< Pointer forwarded to @p event_cb. */
} blusys_usb_device_config_t;

/**
 * @brief Install and start the USB device stack.
 * @param config   Configuration, must be non-NULL.
 * @param out_dev  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without a USB device controller.
 */
blusys_err_t blusys_usb_device_open(const blusys_usb_device_config_t *config,
                                     blusys_usb_device_t **out_dev);

/**
 * @brief Stop the USB device stack and free the handle.
 * @param dev  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p dev is NULL.
 */
blusys_err_t blusys_usb_device_close(blusys_usb_device_t *dev);

/**
 * @brief Write bytes on the CDC IN endpoint. Requires CDC class.
 * @param dev   Handle.
 * @param data  Bytes to send.
 * @param len   Byte count.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not enumerated as CDC,
 *         `BLUSYS_ERR_IO` on transmit failure.
 */
blusys_err_t blusys_usb_device_cdc_write(blusys_usb_device_t *dev,
                                          const void *data, size_t len);

/**
 * @brief Register or replace the CDC receive callback.
 *
 * The callback runs in a USB task context; keep it short and non-blocking.
 *
 * @param dev       Handle.
 * @param callback  RX callback; NULL clears the existing registration.
 * @param user_ctx  Opaque pointer forwarded to @p callback.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not enumerated as CDC.
 */
blusys_err_t blusys_usb_device_cdc_set_rx_callback(
    blusys_usb_device_t *dev,
    blusys_usb_device_cdc_rx_callback_t callback,
    void *user_ctx);

/**
 * @brief Submit a single HID report on the IN endpoint. Requires HID class.
 * @param dev     Handle.
 * @param report  Report bytes, formatted per the active report descriptor.
 * @param len     Report length.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not enumerated as HID,
 *         `BLUSYS_ERR_IO` on transmit failure.
 */
blusys_err_t blusys_usb_device_hid_send_report(blusys_usb_device_t *dev,
                                                 const uint8_t *report,
                                                 size_t len);

#ifdef __cplusplus
}
#endif

#endif
