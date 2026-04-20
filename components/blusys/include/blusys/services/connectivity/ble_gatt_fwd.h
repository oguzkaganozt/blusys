#ifndef BLUSYS_BLE_GATT_FWD_H
#define BLUSYS_BLE_GATT_FWD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ble_gatt_fwd.h
 * @brief Forward declarations for the BLE GATT server types.
 *
 * Lets other headers refer to @ref blusys_ble_gatt_t and its callback signatures
 * without pulling in the full @ref ble_gatt.h definitions.
 */

struct blusys_ble_gatt;
/** @brief Opaque BLE GATT server handle. */
typedef struct blusys_ble_gatt blusys_ble_gatt_t;

/**
 * @brief Called when a BLE client connects or disconnects.
 *
 * @p conn_handle identifies the connection and may be used with
 * @ref blusys_ble_gatt_notify. On disconnect @p connected is false.
 *
 * @param conn_handle  NimBLE connection handle.
 * @param connected    True on connect, false on disconnect.
 * @param user_ctx     The @c conn_user_ctx pointer passed in the config.
 */
typedef void (*blusys_ble_gatt_conn_cb_t)(uint16_t conn_handle, bool connected,
                                           void *user_ctx);

struct blusys_ble_gatt_chr_def;
/** @brief Forward alias for @ref blusys_ble_gatt_chr_def. */
typedef struct blusys_ble_gatt_chr_def blusys_ble_gatt_chr_def_t;

struct blusys_ble_gatt_svc_def;
/** @brief Forward alias for @ref blusys_ble_gatt_svc_def. */
typedef struct blusys_ble_gatt_svc_def blusys_ble_gatt_svc_def_t;

struct blusys_ble_gatt_config;
/** @brief Forward alias for @ref blusys_ble_gatt_config. */
typedef struct blusys_ble_gatt_config blusys_ble_gatt_config_t;

#ifdef __cplusplus
}
#endif

#endif
