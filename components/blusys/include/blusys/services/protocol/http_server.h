/**
 * @file http_server.h
 * @brief Minimal HTTP server with route registration.
 *
 * Routes are registered at ::blusys_http_server_open and dispatched from the
 * `httpd` task. Handlers must send exactly one response via
 * ::blusys_http_server_resp_send. See docs/services/http.md.
 */

#ifndef BLUSYS_HTTP_SERVER_H
#define BLUSYS_HTTP_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"
#include "blusys/services/protocol/http_client.h" /* for blusys_http_method_t */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque HTTP server handle. */
typedef struct blusys_http_server     blusys_http_server_t;

/** @brief Opaque request object; valid only inside a handler callback. */
typedef struct blusys_http_server_req blusys_http_server_req_t;

/**
 * @brief Request handler callback.
 *
 * Called from the httpd task when a matching route is hit. The handler must
 * send exactly one response via ::blusys_http_server_resp_send and return
 * `BLUSYS_OK`. Returning any other value causes httpd to send a 500.
 *
 * @param req       Request object.
 * @param user_ctx  User pointer supplied via ::blusys_http_server_route_t::user_ctx.
 * @return `BLUSYS_OK` on success.
 */
typedef blusys_err_t (*blusys_http_server_handler_t)(blusys_http_server_req_t *req,
                                                      void                     *user_ctx);

/**
 * @brief One URI/method binding.
 *
 * The routes array is copied during open, so the caller need not keep it
 * alive after ::blusys_http_server_open returns.
 */
typedef struct {
    const char                     *uri;      /**< Required; exact match, e.g. `"/api/data"`. */
    blusys_http_method_t            method;   /**< Method to match. */
    blusys_http_server_handler_t    handler;  /**< Handler function. */
    void                           *user_ctx; /**< Opaque pointer forwarded to @ref handler. */
} blusys_http_server_route_t;

/** @brief Configuration for ::blusys_http_server_open. */
typedef struct {
    uint16_t                          port;              /**< Listen port; `0` selects 80. */
    const blusys_http_server_route_t *routes;            /**< Route table. */
    size_t                            route_count;       /**< Number of routes in @ref routes. */
    size_t                            max_open_sockets;  /**< Max concurrent connections; `0` selects 4. */
} blusys_http_server_config_t;

/**
 * @brief Start the HTTP server and register all routes.
 * @param config     Configuration.
 * @param out_handle Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_http_server_open(const blusys_http_server_config_t *config,
                                      blusys_http_server_t             **out_handle);

/**
 * @brief Stop the server and free the handle.
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p handle is NULL.
 */
blusys_err_t blusys_http_server_close(blusys_http_server_t *handle);

/** @brief Whether the server is currently running. */
bool blusys_http_server_is_running(blusys_http_server_t *handle);

/**
 * @brief Return the request URI string.
 *
 * Pointer is owned by the httpd internals and valid only for the duration
 * of the handler call.
 *
 * @param req  Request object.
 */
const char *blusys_http_server_req_uri(blusys_http_server_req_t *req);

/**
 * @brief Return the `Content-Length` of the request body.
 * @param req  Request object.
 * @return Byte count, or `0` if no body.
 */
size_t blusys_http_server_req_content_len(blusys_http_server_req_t *req);

/**
 * @brief Read a chunk of the request body.
 *
 * Call repeatedly until `*out_received == 0` or an error occurs.
 *
 * @param req           Request object.
 * @param buf           Destination buffer.
 * @param buf_len       Capacity of @p buf.
 * @param out_received  Output: bytes actually read.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_http_server_req_recv(blusys_http_server_req_t *req,
                                          uint8_t                  *buf,
                                          size_t                    buf_len,
                                          size_t                   *out_received);

/**
 * @brief Read a single request header into @p out_buf (null-terminated).
 * @param req      Request object.
 * @param name     Header name.
 * @param out_buf  Destination buffer.
 * @param buf_len  Capacity of @p out_buf.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the header is missing or does not fit.
 */
blusys_err_t blusys_http_server_req_get_header(blusys_http_server_req_t *req,
                                                const char               *name,
                                                char                     *out_buf,
                                                size_t                    buf_len);

/**
 * @brief Send an HTTP response. Call exactly once per handler invocation.
 * @param req           Request object.
 * @param status_code   HTTP status (e.g. 200, 404, 500).
 * @param content_type  Optional MIME type; NULL omits the header.
 * @param body          Response body; NULL sends an empty body.
 * @param body_len      Body length; pass `-1` to call `strlen(body)`.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_http_server_resp_send(blusys_http_server_req_t *req,
                                           int                       status_code,
                                           const char               *content_type,
                                           const char               *body,
                                           int                       body_len);

#ifdef __cplusplus
}
#endif

#endif
