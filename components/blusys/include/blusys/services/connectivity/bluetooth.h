/**
 * @file bluetooth.h
 * @brief BLE advertising and scanning via the NimBLE host stack.
 *
 * Offers a minimal peripheral (advertising) and observer (scanning) surface on
 * top of NimBLE. No GATT server, no connection establishment — those live in
 * @ref ble_gatt.h. Classic Bluetooth (BR/EDR) is not exposed.
 *
 * Coexistence: only one BLE-owning module may be open at a time. A second
 * opener of any of the following receives BLUSYS_ERR_INVALID_STATE:
 *   `blusys_bluetooth`, `blusys_ble_gatt`, BLE-transport `blusys_usb_hid`,
 *   BLE-transport `blusys_wifi_prov`, `blusys_ble_hid_device`.
 */

#ifndef BLUSYS_BLUETOOTH_H
#define BLUSYS_BLUETOOTH_H

#include <stdint.h>
#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open BLE session. */
typedef struct blusys_bluetooth blusys_bluetooth_t;

/** @brief Configuration passed to @ref blusys_bluetooth_open. */
typedef struct {
    const char *device_name;  /**< BLE GAP name broadcast in advertising data (required, ≤29 bytes). */
} blusys_bluetooth_config_t;

/** @brief One advertising report delivered to the scan callback. */
typedef struct {
    char    name[32];   /**< Device name from the Complete Local Name AD structure; empty if absent. */
    uint8_t addr[6];    /**< BLE address bytes in wire order (index 0 is LSB). */
    int8_t  rssi;       /**< Received signal strength in dBm. */
} blusys_bluetooth_scan_result_t;

/**
 * @brief Called from the NimBLE host task for each advertising report received.
 *
 * The @p result pointer is valid only for the duration of the call — copy any
 * fields you need before returning.
 *
 * @param result    Advertising report.
 * @param user_ctx  The @c user_ctx pointer passed to @ref blusys_bluetooth_scan_start.
 */
typedef void (*blusys_bluetooth_scan_cb_t)(const blusys_bluetooth_scan_result_t *result,
                                            void *user_ctx);

/**
 * @brief Initialize the NimBLE stack and return a session handle.
 *
 * Blocks up to 5 s waiting for the NimBLE host to synchronize with the controller.
 * Only one handle may be open at a time — a second call returns BLUSYS_ERR_INVALID_STATE
 * until @ref blusys_bluetooth_close is called.
 *
 * @param config  Device name (required).
 * @param out_bt  Output handle.
 *
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG for invalid config,
 *         BLUSYS_ERR_INVALID_STATE if already open or another BLE owner holds the stack,
 *         BLUSYS_ERR_TIMEOUT if host sync fails, BLUSYS_ERR_NO_MEM on allocation failure.
 */
blusys_err_t blusys_bluetooth_open(const blusys_bluetooth_config_t *config,
                                    blusys_bluetooth_t **out_bt);

/**
 * @brief Stop advertising / scanning (if active), shut down NimBLE, free the handle.
 *
 * @param bt  Handle returned by @ref blusys_bluetooth_open.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_bluetooth_close(blusys_bluetooth_t *bt);

/**
 * @brief Begin undirected general BLE advertising using the configured device name.
 *
 * Advertising continues until @ref blusys_bluetooth_advertise_stop or close.
 *
 * @param bt  Open session handle.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_STATE if already advertising.
 */
blusys_err_t blusys_bluetooth_advertise_start(blusys_bluetooth_t *bt);

/**
 * @brief Stop BLE advertising.
 *
 * @param bt  Open session handle.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_STATE if not currently advertising.
 */
blusys_err_t blusys_bluetooth_advertise_stop(blusys_bluetooth_t *bt);

/**
 * @brief Start passive BLE scanning.
 *
 * For each advertising device found, @p cb is invoked from the NimBLE host task.
 * Duplicate reports from the same address are suppressed.
 *
 * @param bt        Open session handle.
 * @param cb        Scan callback (required).
 * @param user_ctx  Passed unchanged to each @p cb invocation.
 *
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_STATE if already scanning.
 */
blusys_err_t blusys_bluetooth_scan_start(blusys_bluetooth_t *bt,
                                          blusys_bluetooth_scan_cb_t cb,
                                          void *user_ctx);

/**
 * @brief Stop BLE scanning.
 *
 * @param bt  Open session handle.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_STATE if not currently scanning.
 */
blusys_err_t blusys_bluetooth_scan_stop(blusys_bluetooth_t *bt);

#ifdef __cplusplus
}
#endif

#endif
