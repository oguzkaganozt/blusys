#ifndef BLUSYS_HTTP_SERVER_H
#define BLUSYS_HTTP_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"
#include "blusys/http_client.h" /* for blusys_http_method_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_http_server     blusys_http_server_t;
typedef struct blusys_http_server_req blusys_http_server_req_t; /* opaque; valid only inside handler */

/* Called from the httpd task when a matching request arrives.
   The handler must send exactly one response via blusys_http_server_resp_send()
   and return BLUSYS_OK.  Returning any other value causes httpd to send a 500. */
typedef blusys_err_t (*blusys_http_server_handler_t)(blusys_http_server_req_t *req,
                                                      void                     *user_ctx);

/* A single URI/method binding.  The routes array must remain valid for the
   lifetime of the server handle (the pointer is not copied). */
typedef struct {
    const char                     *uri;      /* required; exact match, e.g. "/api/data" */
    blusys_http_method_t            method;
    blusys_http_server_handler_t    handler;
    void                           *user_ctx; /* passed through to handler; NULL is fine */
} blusys_http_server_route_t;

typedef struct {
    uint16_t                          port;             /* listen port; 0 = default 80 */
    const blusys_http_server_route_t *routes;           /* array of route definitions */
    size_t                            route_count;
    size_t                            max_open_sockets; /* max concurrent connections; 0 = default (4) */
} blusys_http_server_config_t;

/* Start the HTTP server and register all routes.
   Returns BLUSYS_ERR_INVALID_ARG if config or out_handle is NULL, or if
   route_count > 0 but routes is NULL.
   Returns BLUSYS_ERR_NO_MEM on allocation failure.
   Returns BLUSYS_ERR_NOT_SUPPORTED on targets without WiFi. */
blusys_err_t blusys_http_server_open(const blusys_http_server_config_t *config,
                                      blusys_http_server_t             **out_handle);

/* Stop the server and free the handle.  After this call the handle is invalid. */
blusys_err_t blusys_http_server_close(blusys_http_server_t *handle);

/* Returns true if the server is currently running. */
bool blusys_http_server_is_running(blusys_http_server_t *handle);

/* --- Request accessors (valid only inside a handler callback) --- */

/* Returns the request URI string (null-terminated, pointer into httpd internals). */
const char *blusys_http_server_req_uri(blusys_http_server_req_t *req);

/* Returns the Content-Length of the request body (0 if no body). */
size_t blusys_http_server_req_content_len(blusys_http_server_req_t *req);

/* Read up to buf_len bytes of the request body into buf.
   Sets *out_received to the number of bytes actually read.
   Call repeatedly until *out_received == 0 or an error occurs. */
blusys_err_t blusys_http_server_req_recv(blusys_http_server_req_t *req,
                                          uint8_t                  *buf,
                                          size_t                    buf_len,
                                          size_t                   *out_received);

/* Read the value of a request header into out_buf (null-terminated).
   Returns BLUSYS_ERR_IO if the header is not present or the value does not fit in out_buf. */
blusys_err_t blusys_http_server_req_get_header(blusys_http_server_req_t *req,
                                                const char               *name,
                                                char                     *out_buf,
                                                size_t                    buf_len);

/* --- Response sender --- */

/* Send an HTTP response.  Call exactly once per handler invocation.
   status_code: e.g. 200, 404, 500.
   content_type: MIME type string, e.g. "text/plain" or "application/json"; NULL omits the header.
   body: response body; NULL sends an empty body.
   body_len: byte count; pass -1 to use strlen(body). */
blusys_err_t blusys_http_server_resp_send(blusys_http_server_req_t *req,
                                           int                       status_code,
                                           const char               *content_type,
                                           const char               *body,
                                           int                       body_len);

#ifdef __cplusplus
}
#endif

#endif
