#ifndef BLUSYS_ESPNOW_H
#define BLUSYS_ESPNOW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file espnow.h
 * @brief ESP-NOW connectionless peer-to-peer wireless (no access point required).
 *
 * Initializes the WiFi driver in STA mode internally — do not combine with
 * `blusys_wifi`, `blusys_wifi_mesh`, or `blusys_wifi_prov` in the same
 * application. Maximum payload per frame is 250 bytes.
 */

/** @brief Opaque handle to an open ESP-NOW session. */
typedef struct blusys_espnow blusys_espnow_t;

/**
 * Called when data is received from a peer.
 *
 * Invoked from the WiFi task. The @p data pointer is valid only for the
 * duration of the callback — copy any bytes you need before returning.
 *
 * @param peer_addr  MAC address of the sender (6 bytes).
 * @param data       Payload bytes.
 * @param len        Number of payload bytes.
 * @param user_ctx   Value passed as @c recv_user_ctx in the config.
 */
typedef void (*blusys_espnow_recv_cb_t)(const uint8_t peer_addr[6],
                                        const uint8_t *data, size_t len,
                                        void *user_ctx);

/**
 * Called when a previously requested send completes (optional).
 *
 * @param peer_addr  MAC address of the destination peer (6 bytes).
 * @param success    True if the frame was acknowledged, false otherwise.
 * @param user_ctx   Value passed as @c send_user_ctx in the config.
 */
typedef void (*blusys_espnow_send_cb_t)(const uint8_t peer_addr[6],
                                        bool success, void *user_ctx);

/** Configuration passed to blusys_espnow_open(). */
typedef struct {
    uint8_t                  channel;        /**< WiFi channel; 0 = use current */
    blusys_espnow_recv_cb_t  recv_cb;        /**< Receive callback (required) */
    void                    *recv_user_ctx;  /**< Passed to recv_cb */
    blusys_espnow_send_cb_t  send_cb;        /**< Send-complete callback (optional) */
    void                    *send_user_ctx;  /**< Passed to send_cb */
} blusys_espnow_config_t;

/** Peer descriptor passed to blusys_espnow_add_peer(). */
typedef struct {
    uint8_t addr[6];   /**< Peer MAC address */
    uint8_t channel;   /**< 0 = same channel as sender */
    bool    encrypt;   /**< Enable CCMP encryption */
    uint8_t lmk[16];   /**< Local master key (used only when encrypt = true) */
} blusys_espnow_peer_t;

/**
 * Initializes WiFi in STA mode, starts ESP-NOW, and returns a handle.
 *
 * Only one handle may be open at a time. A second call returns
 * BLUSYS_ERR_INVALID_STATE until blusys_espnow_close() is called.
 *
 * @param config     Configuration (recv_cb required, all other fields optional).
 * @param out_handle Output handle.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_espnow_open(const blusys_espnow_config_t *config,
                                blusys_espnow_t **out_handle);

/**
 * Stops ESP-NOW, shuts down WiFi, and frees the handle.
 *
 * @param handle  Handle returned by blusys_espnow_open().
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_espnow_close(blusys_espnow_t *handle);

/**
 * Registers a peer so that frames can be sent to its MAC address.
 *
 * @param handle  Open ESP-NOW handle.
 * @param peer    Peer descriptor.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG if the peer is
 *         already registered or arguments are invalid.
 */
blusys_err_t blusys_espnow_add_peer(blusys_espnow_t *handle,
                                    const blusys_espnow_peer_t *peer);

/**
 * Removes a previously registered peer.
 *
 * @param handle  Open ESP-NOW handle.
 * @param addr    MAC address of the peer to remove (6 bytes).
 * @return BLUSYS_OK on success, BLUSYS_ERR_NOT_FOUND if addr is unknown.
 */
blusys_err_t blusys_espnow_remove_peer(blusys_espnow_t *handle,
                                       const uint8_t addr[6]);

/**
 * Sends data to a registered peer (non-blocking).
 *
 * The send callback (if configured) is invoked once delivery is confirmed
 * or fails. Maximum payload is 250 bytes.
 *
 * @param handle  Open ESP-NOW handle.
 * @param addr    Destination MAC address (6 bytes); must be a registered peer.
 * @param data    Payload to send.
 * @param size    Number of bytes to send (max 250).
 * @return BLUSYS_OK if the frame was accepted for transmission,
 *         BLUSYS_ERR_INVALID_ARG if size > 250 or args are NULL.
 */
blusys_err_t blusys_espnow_send(blusys_espnow_t *handle,
                                const uint8_t addr[6],
                                const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
