#ifndef BLUSYS_BLE_GATT_H
#define BLUSYS_BLE_GATT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque BLE GATT server handle.
 */
typedef struct blusys_ble_gatt blusys_ble_gatt_t;

/** Characteristic can be read by a connected client. */
#define BLUSYS_BLE_GATT_CHR_F_READ    (1u << 0)
/** Characteristic can be written by a connected client. */
#define BLUSYS_BLE_GATT_CHR_F_WRITE   (1u << 1)
/** Characteristic supports notifications (CCCD added automatically). */
#define BLUSYS_BLE_GATT_CHR_F_NOTIFY  (1u << 2)

/**
 * @brief Called when a connected client reads a characteristic.
 *
 * Write up to @p max_len bytes into @p out_data and set @p *out_len to the
 * actual length. Return 0 on success or an ATT error code on failure.
 */
typedef int (*blusys_ble_gatt_read_cb_t)(uint16_t conn_handle,
                                          const uint8_t uuid[16],
                                          uint8_t *out_data, size_t max_len,
                                          size_t *out_len, void *user_ctx);

/**
 * @brief Called when a connected client writes a characteristic.
 *
 * Return 0 on success or an ATT error code on failure.
 */
typedef int (*blusys_ble_gatt_write_cb_t)(uint16_t conn_handle,
                                           const uint8_t uuid[16],
                                           const uint8_t *data, size_t len,
                                           void *user_ctx);

/**
 * @brief Called when a BLE client connects or disconnects.
 *
 * @p conn_handle identifies the connection and may be used with
 * @ref blusys_ble_gatt_notify. On disconnect @p connected is false.
 */
typedef void (*blusys_ble_gatt_conn_cb_t)(uint16_t conn_handle, bool connected,
                                           void *user_ctx);

/**
 * @brief Characteristic definition.
 *
 * Embed one of these per characteristic in a @ref blusys_ble_gatt_svc_def_t.
 * The struct must remain valid for the lifetime of the GATT handle.
 */
typedef struct {
    uint8_t                      uuid[16];        /**< 128-bit UUID, little-endian byte order. */
    uint32_t                     flags;            /**< @ref BLUSYS_BLE_GATT_CHR_F_* bitmask. */
    blusys_ble_gatt_read_cb_t    read_cb;          /**< Required when BLUSYS_BLE_GATT_CHR_F_READ is set. */
    blusys_ble_gatt_write_cb_t   write_cb;         /**< Required when BLUSYS_BLE_GATT_CHR_F_WRITE is set. */
    void                        *user_ctx;         /**< Passed to read_cb / write_cb as-is. */
    uint16_t                    *val_handle_out;   /**< Filled by blusys_ble_gatt_open(); required when BLUSYS_BLE_GATT_CHR_F_NOTIFY is set. */
} blusys_ble_gatt_chr_def_t;

/**
 * @brief Service definition.
 *
 * Groups one or more characteristics under a single primary service UUID.
 */
typedef struct {
    uint8_t                           uuid[16];         /**< 128-bit service UUID, little-endian. */
    const blusys_ble_gatt_chr_def_t  *characteristics;  /**< Array of chr_count characteristic definitions. */
    size_t                            chr_count;         /**< Number of entries in @p characteristics. */
} blusys_ble_gatt_svc_def_t;

/**
 * @brief Configuration passed to @ref blusys_ble_gatt_open.
 */
typedef struct {
    const char                       *device_name;      /**< BLE advertised device name (max 29 bytes; longer names are rejected). */
    const blusys_ble_gatt_svc_def_t  *services;         /**< Array of svc_count service definitions. */
    size_t                            svc_count;         /**< Number of entries in @p services. */
    blusys_ble_gatt_conn_cb_t         conn_cb;          /**< Optional connection/disconnection callback. */
    void                             *conn_user_ctx;     /**< Passed to conn_cb as-is. */
} blusys_ble_gatt_config_t;

/**
 * @brief Initialize and start a BLE GATT server.
 *
 * Initializes the NimBLE stack, registers the provided services, and begins
 * advertising so that clients can discover and connect to the device.
 *
 * @param config    Server configuration. Must remain valid for the lifetime of the handle.
 * @param out_handle Receives the allocated handle on success.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG for invalid configuration,
 *         BLUSYS_ERR_INVALID_STATE if already open,
 *         BLUSYS_ERR_NO_MEM, BLUSYS_ERR_TIMEOUT, or BLUSYS_ERR_INTERNAL on failure.
 */
blusys_err_t blusys_ble_gatt_open(const blusys_ble_gatt_config_t *config,
                                   blusys_ble_gatt_t **out_handle);

/**
 * @brief Stop advertising and shut down the GATT server.
 *
 * @param handle Handle returned by blusys_ble_gatt_open().
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG if handle is NULL.
 */
blusys_err_t blusys_ble_gatt_close(blusys_ble_gatt_t *handle);

/**
 * @brief Send a notification for a characteristic to a connected client.
 *
 * @param handle          Handle returned by blusys_ble_gatt_open().
 * @param conn_handle     Connection handle from the conn_cb.
 * @param chr_val_handle  Characteristic value handle (stored in val_handle_out on open).
 * @param data            Notification payload.
 * @param len             Payload length in bytes.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG if handle/data is NULL,
 *         BLUSYS_ERR_INVALID_STATE if the server is closing, BLUSYS_ERR_NO_MEM
 *         if buffer allocation fails, BLUSYS_ERR_INTERNAL on stack error.
 */
blusys_err_t blusys_ble_gatt_notify(blusys_ble_gatt_t *handle,
                                     uint16_t conn_handle,
                                     uint16_t chr_val_handle,
                                     const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
