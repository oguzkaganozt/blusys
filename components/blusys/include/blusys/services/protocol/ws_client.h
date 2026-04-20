/**
 * @file ws_client.h
 * @brief WebSocket client (`ws://` and `wss://`) with TEXT/BINARY frames.
 *
 * Callbacks run on the receive task; no fragmentation reassembly is
 * performed — the receive callback fires per frame. See
 * docs/services/ws_client.md.
 */

#ifndef BLUSYS_WS_CLIENT_H
#define BLUSYS_WS_CLIENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque WebSocket client handle. */
typedef struct blusys_ws_client blusys_ws_client_t;

/** @brief WebSocket data-frame type. */
typedef enum {
    BLUSYS_WS_MSG_TEXT   = 0,   /**< UTF-8 text frame. */
    BLUSYS_WS_MSG_BINARY = 1,   /**< Binary frame. */
} blusys_ws_msg_type_t;

/**
 * @brief Incoming-frame callback.
 *
 * May fire per fragment; no reassembly is performed. For TEXT frames @p data
 * is not null-terminated — use @p len.
 *
 * @param type      Frame type.
 * @param data      Frame bytes.
 * @param len       Number of bytes in @p data.
 * @param user_ctx  User pointer supplied via ::blusys_ws_client_config_t::user_ctx.
 */
typedef void (*blusys_ws_message_cb_t)(blusys_ws_msg_type_t  type,
                                        const uint8_t        *data,
                                        size_t                len,
                                        void                 *user_ctx);

/**
 * @brief Disconnect callback. Fired when the server closes the connection
 *        or a network error occurs.
 * @param user_ctx  User pointer supplied via ::blusys_ws_client_config_t::user_ctx.
 */
typedef void (*blusys_ws_disconnect_cb_t)(void *user_ctx);

/** @brief Configuration for ::blusys_ws_client_open. */
typedef struct {
    const char                  *url;             /**< Required `ws://` or `wss://` URL. */
    blusys_ws_message_cb_t       message_cb;      /**< Required incoming-frame callback. */
    blusys_ws_disconnect_cb_t    disconnect_cb;   /**< Optional disconnect callback. */
    void                        *user_ctx;        /**< Opaque pointer forwarded to both callbacks. */
    int                          timeout_ms;      /**< Connect/send timeout; `BLUSYS_TIMEOUT_FOREVER` to block. */
    const char                  *subprotocol;     /**< Optional `Sec-WebSocket-Protocol` value. */
    const char                  *server_cert_pem; /**< PEM CA cert for wss://; NULL skips CN check. */
} blusys_ws_client_config_t;

/**
 * @brief Allocate and initialise a WebSocket client. Does not connect.
 * @param config      Configuration.
 * @param out_handle  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_ws_client_open(const blusys_ws_client_config_t *config,
                                    blusys_ws_client_t             **out_handle);

/**
 * @brief Disconnect (if connected) and free the client.
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p handle is NULL.
 */
blusys_err_t blusys_ws_client_close(blusys_ws_client_t *handle);

/**
 * @brief Connect to the server.
 *
 * Blocks until the WebSocket handshake completes, the timeout expires, or
 * a network error occurs.
 *
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if already connected,
 *         `BLUSYS_ERR_IO` on network-level failure.
 */
blusys_err_t blusys_ws_client_connect(blusys_ws_client_t *handle);

/**
 * @brief Send a WebSocket CLOSE and tear down the transport.
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_ws_client_disconnect(blusys_ws_client_t *handle);

/**
 * @brief Send a UTF-8 text frame.
 * @param handle  Handle.
 * @param text    Null-terminated UTF-8 string; must not be NULL.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not connected.
 */
blusys_err_t blusys_ws_client_send_text(blusys_ws_client_t *handle, const char *text);

/**
 * @brief Send a binary frame.
 * @param handle  Handle.
 * @param data    Payload bytes.
 * @param len     Bytes in @p data.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not connected.
 */
blusys_err_t blusys_ws_client_send(blusys_ws_client_t *handle,
                                    const uint8_t      *data,
                                    size_t              len);

/** @brief Whether the client is currently connected. */
bool blusys_ws_client_is_connected(blusys_ws_client_t *handle);

#ifdef __cplusplus
}
#endif

#endif
