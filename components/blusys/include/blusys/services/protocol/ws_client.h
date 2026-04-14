#ifndef BLUSYS_WS_CLIENT_H
#define BLUSYS_WS_CLIENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_ws_client blusys_ws_client_t;

typedef enum {
    BLUSYS_WS_MSG_TEXT   = 0,
    BLUSYS_WS_MSG_BINARY = 1,
} blusys_ws_msg_type_t;

/* Called from the receive task when a data frame arrives.
   data is raw bytes; for TEXT frames it is NOT null-terminated.
   This callback may be called for fragmented frames individually — no reassembly
   is performed.  Check message type via the type parameter. */
typedef void (*blusys_ws_message_cb_t)(blusys_ws_msg_type_t  type,
                                        const uint8_t        *data,
                                        size_t                len,
                                        void                 *user_ctx);

/* Called from the receive task when the connection is closed by the server
   or a network error occurs.  Optional; set to NULL if not needed. */
typedef void (*blusys_ws_disconnect_cb_t)(void *user_ctx);

typedef struct {
    const char                  *url;            /* required; "ws://" or "wss://" */
    blusys_ws_message_cb_t       message_cb;     /* required; called on incoming data frames */
    blusys_ws_disconnect_cb_t    disconnect_cb;  /* optional; called on disconnect */
    void                        *user_ctx;       /* passed through to callbacks */
    int                          timeout_ms;     /* connect/send timeout; BLUSYS_TIMEOUT_FOREVER to block */
    const char                  *subprotocol;    /* optional Sec-WebSocket-Protocol value */
    const char                  *server_cert_pem; /* PEM CA cert for wss://; NULL skips CN check */
} blusys_ws_client_config_t;

/* Allocate and initialise a WebSocket client.  Does not connect.
   Returns BLUSYS_ERR_INVALID_ARG if config or out_handle is NULL, url or message_cb is NULL,
   or timeout_ms is invalid (less than BLUSYS_TIMEOUT_FOREVER).
   Returns BLUSYS_ERR_NO_MEM on allocation failure.
   Returns BLUSYS_ERR_NOT_SUPPORTED on targets without WiFi. */
blusys_err_t blusys_ws_client_open(const blusys_ws_client_config_t *config,
                                    blusys_ws_client_t             **out_handle);

/* Release all resources.  Disconnects first if still connected. */
blusys_err_t blusys_ws_client_close(blusys_ws_client_t *handle);

/* Connect to the server.  Blocks until the WebSocket handshake completes, the
   timeout expires, or a network error occurs.
   Returns BLUSYS_ERR_INVALID_STATE if already connected.
   Returns BLUSYS_ERR_IO on network-level failure. */
blusys_err_t blusys_ws_client_connect(blusys_ws_client_t *handle);

/* Disconnect from the server.  Sends a WebSocket CLOSE frame, waits for the
   receive task to finish, then tears down the transport. */
blusys_err_t blusys_ws_client_disconnect(blusys_ws_client_t *handle);

/* Send a UTF-8 text frame.  null must not be passed as text.
   Returns BLUSYS_ERR_INVALID_STATE if not connected. */
blusys_err_t blusys_ws_client_send_text(blusys_ws_client_t *handle, const char *text);

/* Send a binary frame.
   Returns BLUSYS_ERR_INVALID_STATE if not connected. */
blusys_err_t blusys_ws_client_send(blusys_ws_client_t *handle,
                                    const uint8_t      *data,
                                    size_t              len);

/* Returns true if the client is currently connected. */
bool blusys_ws_client_is_connected(blusys_ws_client_t *handle);

#ifdef __cplusplus
}
#endif

#endif
