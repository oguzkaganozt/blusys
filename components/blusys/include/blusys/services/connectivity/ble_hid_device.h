/**
 * @file ble_hid_device.h
 * @brief BLE HID (HOGP) peripheral service — consumer-control remote.
 *
 * Registers a standards-compliant HID-over-GATT peripheral: HID Service
 * (0x1812) with a Report Map that declares an 8-bit consumer-control report,
 * plus Battery Service (0x180F) and Device Information Service (0x180A).
 * Any modern OS pairs with it natively as a HID media device — no companion
 * app required.
 *
 * Coexistence: shares the BLE controller with the rest of the NimBLE-based
 * services so it mutually excludes `blusys_bluetooth`, `blusys_ble_gatt`,
 * Wi-Fi BLE provisioning, and the USB-HID BLE-central path. Only one
 * BLE-owning module may be open at a time.
 *
 * Report bit layout (1-byte payload on the Input Report characteristic):
 *   bit 0 → Usage 0x00E9 (Consumer: Volume Increment)
 *   bit 1 → Usage 0x00EA (Consumer: Volume Decrement)
 *   bit 2 → Usage 0x00E2 (Consumer: Mute)
 *   bit 3 → Usage 0x00CD (Consumer: Play/Pause)
 *   bit 4 → Usage 0x00B5 (Consumer: Next Track)
 *   bit 5 → Usage 0x00B6 (Consumer: Previous Track)
 *   bit 6 → Usage 0x006F (Consumer: Brightness Up)
 *   bit 7 → Usage 0x0070 (Consumer: Brightness Down)
 */

#ifndef BLUSYS_BLE_HID_DEVICE_H
#define BLUSYS_BLE_HID_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open BLE HID peripheral session. */
typedef struct blusys_ble_hid_device blusys_ble_hid_device_t;

/**
 * @brief Connect / disconnect callback.
 *
 * @param connected  True on connect, false on disconnect.
 * @param user_ctx   The @c conn_user_ctx pointer passed in the config.
 */
typedef void (*blusys_ble_hid_device_conn_cb_t)(bool connected, void *user_ctx);

/** @brief Configuration passed to @ref blusys_ble_hid_device_open. */
typedef struct {
    const char *device_name;            /**< Advertised name (required; ≤29 bytes). */
    uint16_t    vendor_id;              /**< PnP ID: USB-IF vendor id (0 → default 0x05AC). */
    uint16_t    product_id;             /**< PnP ID: product id (0 → default 0x820A). */
    uint16_t    version;                /**< PnP ID: product version (0 → default 0x0100). */
    uint8_t     initial_battery_pct;    /**< Battery Level initial value, 0–100 (values >100 clamped). */
    blusys_ble_hid_device_conn_cb_t conn_cb; /**< Optional connect/disconnect callback. */
    void       *conn_user_ctx;          /**< Passed to conn_cb. */
} blusys_ble_hid_device_config_t;

/** Consumer: Volume Increment. */
#define BLUSYS_HID_USAGE_VOLUME_UP       0x00E9u
/** Consumer: Volume Decrement. */
#define BLUSYS_HID_USAGE_VOLUME_DOWN     0x00EAu
/** Consumer: Mute. */
#define BLUSYS_HID_USAGE_MUTE            0x00E2u
/** Consumer: Play/Pause. */
#define BLUSYS_HID_USAGE_PLAY_PAUSE      0x00CDu
/** Consumer: Scan Next Track. */
#define BLUSYS_HID_USAGE_NEXT_TRACK      0x00B5u
/** Consumer: Scan Previous Track. */
#define BLUSYS_HID_USAGE_PREV_TRACK      0x00B6u
/** Consumer: Brightness Up. */
#define BLUSYS_HID_USAGE_BRIGHTNESS_UP   0x006Fu
/** Consumer: Brightness Down. */
#define BLUSYS_HID_USAGE_BRIGHTNESS_DOWN 0x0070u

/**
 * @brief Initialize the HID peripheral and begin advertising.
 *
 * Acquires the BLE controller, registers the three SIG services, configures
 * Just-Works bonding, and starts connectable advertising.
 *
 * @return BLUSYS_OK, BLUSYS_ERR_INVALID_ARG, BLUSYS_ERR_INVALID_STATE,
 *         BLUSYS_ERR_BUSY (another BLE owner active), BLUSYS_ERR_NO_MEM,
 *         BLUSYS_ERR_TIMEOUT, BLUSYS_ERR_INTERNAL, BLUSYS_ERR_NOT_SUPPORTED.
 */
blusys_err_t blusys_ble_hid_device_open(const blusys_ble_hid_device_config_t *config,
                                         blusys_ble_hid_device_t **out_handle);

/** Stop advertising, disconnect, release the BLE controller. */
blusys_err_t blusys_ble_hid_device_close(blusys_ble_hid_device_t *handle);

/** Returns true if a client is currently connected. */
bool blusys_ble_hid_device_is_connected(const blusys_ble_hid_device_t *handle);

/**
 * @brief Returns true if a client is connected **and** has subscribed to the
 *        Input Report characteristic.
 *
 * Only when this is true can consumer reports actually reach the host OS.
 * Use this (not is_connected) to gate UX that requires reports to land —
 * simulators, demo sequences, or app-level "ready" indicators.
 */
bool blusys_ble_hid_device_is_ready(const blusys_ble_hid_device_t *handle);

/**
 * @brief Edge-triggered consumer-control report update.
 *
 * Sets (pressed=true) or clears (pressed=false) the bit matching @p usage and
 * sends a notification on the Input Report characteristic. Returns
 * BLUSYS_ERR_INVALID_ARG for an unsupported usage code, BLUSYS_ERR_INVALID_STATE
 * when no client is connected or the client has not yet subscribed to the
 * Input Report characteristic (use blusys_ble_hid_device_is_ready() to check).
 */
blusys_err_t blusys_ble_hid_device_send_consumer(blusys_ble_hid_device_t *handle,
                                                  uint16_t usage, bool pressed);

/**
 * @brief Send raw bytes on the Input Report characteristic.
 *
 * Escape hatch for future report maps. @p data length must match the report
 * the descriptor declares; callers maintain their own state.
 */
blusys_err_t blusys_ble_hid_device_send_report(blusys_ble_hid_device_t *handle,
                                                const uint8_t *data, size_t len);

/** Update the advertised Battery Level (clamped to 0–100). */
blusys_err_t blusys_ble_hid_device_set_battery(blusys_ble_hid_device_t *handle,
                                                uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif
