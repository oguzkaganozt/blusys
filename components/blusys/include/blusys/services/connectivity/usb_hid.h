#ifndef BLUSYS_USB_HID_H
#define BLUSYS_USB_HID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file usb_hid.h
 * @brief Dual-transport HID input service (USB OTG host on S3, BLE HOGP central elsewhere).
 *
 * Parses boot-protocol keyboard and mouse reports and delivers them through a
 * unified callback. Payloads that do not fit the boot-protocol layouts are
 * surfaced as raw reports so applications can implement their own parsers
 * (e.g. for gamepads).
 *
 * Coexistence: the BLE transport shares the NimBLE controller with other
 * BLE-owning modules. Only one BLE owner may be open at a time.
 */

/** @brief Opaque HID session handle. */
typedef struct blusys_usb_hid blusys_usb_hid_t;

/** @brief Transport used to reach the HID device. */
typedef enum {
    BLUSYS_USB_HID_TRANSPORT_USB,   /**< ESP32-S3 only (USB OTG host + @c usb_host_hid driver). */
    BLUSYS_USB_HID_TRANSPORT_BLE,   /**< All targets (NimBLE HOGP client). */
} blusys_usb_hid_transport_t;

/** @brief HID boot-protocol keyboard report. */
typedef struct {
    uint8_t modifier;       /**< Bitmask of modifier keys (Ctrl, Shift, Alt, GUI). */
    uint8_t keycodes[6];    /**< Up to six simultaneously pressed USB HID key codes. */
} blusys_usb_hid_keyboard_report_t;

/** @brief HID boot-protocol mouse report. */
typedef struct {
    uint8_t buttons;    /**< Bitmask of pressed mouse buttons. */
    int8_t  x;          /**< Signed relative X movement. */
    int8_t  y;          /**< Signed relative Y movement. */
    int8_t  wheel;      /**< Signed relative wheel delta. */
} blusys_usb_hid_mouse_report_t;

/** @brief Raw HID report for devices that do not fit the boot-protocol layouts. */
typedef struct {
    const uint8_t *data;    /**< Payload pointer; valid only for the duration of the callback. */
    size_t         len;     /**< Payload length in bytes. */
} blusys_usb_hid_raw_report_t;

/** @brief Event identifier carried in @ref blusys_usb_hid_event_data_t. */
typedef enum {
    BLUSYS_USB_HID_EVENT_CONNECTED,         /**< Transport linked to a device. */
    BLUSYS_USB_HID_EVENT_DISCONNECTED,      /**< Device removed / link dropped. */
    BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT,   /**< Parsed boot-protocol keyboard report. */
    BLUSYS_USB_HID_EVENT_MOUSE_REPORT,      /**< Parsed boot-protocol mouse report. */
    BLUSYS_USB_HID_EVENT_RAW_REPORT,        /**< Unparsed raw report. */
} blusys_usb_hid_event_t;

/** @brief Payload delivered to the HID callback. Access the union member matching @p event. */
typedef struct {
    blusys_usb_hid_event_t event;   /**< Event identifier. */
    union {
        blusys_usb_hid_keyboard_report_t keyboard;  /**< Valid on KEYBOARD_REPORT. */
        blusys_usb_hid_mouse_report_t    mouse;     /**< Valid on MOUSE_REPORT. */
        blusys_usb_hid_raw_report_t      raw;       /**< Valid on RAW_REPORT. */
    } data;                          /**< Event-specific payload. */
} blusys_usb_hid_event_data_t;

/**
 * @brief HID event callback.
 *
 * Invoked from the USB HID task (USB transport) or the NimBLE host task (BLE transport).
 * Do not call @ref blusys_usb_hid_close from within it.
 *
 * @param hid       Session handle that produced the event.
 * @param event     Event payload; the @p data member is valid only while the callback runs.
 * @param user_ctx  The @c user_ctx pointer passed in the config.
 */
typedef void (*blusys_usb_hid_callback_t)(
    blusys_usb_hid_t *hid,
    const blusys_usb_hid_event_data_t *event,
    void *user_ctx);

/** @brief Configuration passed to @ref blusys_usb_hid_open. */
typedef struct {
    blusys_usb_hid_transport_t transport;       /**< USB or BLE transport. */
    blusys_usb_hid_callback_t  callback;        /**< Event callback (required). */
    void                      *user_ctx;        /**< Passed back in every callback. */
    const char                *ble_device_name; /**< BLE only: filter by peripheral name (NULL = any HID device). */
} blusys_usb_hid_config_t;

/**
 * @brief Open the HID service on the requested transport.
 *
 * For BLE, starts NimBLE and begins scanning for HID peripherals. For USB,
 * installs the USB host stack and HID class driver. Only one instance may be
 * open at a time.
 *
 * @param config   Transport, callback, and optional filters.
 * @param out_hid  Output handle.
 *
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG for invalid config,
 *         BLUSYS_ERR_INVALID_STATE if already open, BLUSYS_ERR_NOT_SUPPORTED if
 *         the transport is unavailable, BLUSYS_ERR_TIMEOUT if BLE sync fails.
 */
blusys_err_t blusys_usb_hid_open(const blusys_usb_hid_config_t *config,
                                  blusys_usb_hid_t **out_hid);

/**
 * @brief Close the HID service and release its transport.
 *
 * @param hid  Handle returned by @ref blusys_usb_hid_open.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_usb_hid_close(blusys_usb_hid_t *hid);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_HID_H */
