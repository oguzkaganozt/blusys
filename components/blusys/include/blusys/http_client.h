#ifndef BLUSYS_HTTP_CLIENT_H
#define BLUSYS_HTTP_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_http_client blusys_http_client_t;

typedef enum {
    BLUSYS_HTTP_METHOD_GET    = 0,
    BLUSYS_HTTP_METHOD_POST,
    BLUSYS_HTTP_METHOD_PUT,
    BLUSYS_HTTP_METHOD_PATCH,
    BLUSYS_HTTP_METHOD_DELETE,
    BLUSYS_HTTP_METHOD_HEAD,
} blusys_http_method_t;

/* Null-terminated name/value pair for request headers.
   Terminate the array with { NULL, NULL }. */
typedef struct {
    const char *name;
    const char *value;
} blusys_http_header_t;

typedef struct {
    const char *url;                      /* required; copied internally at open time */
    const char *cert_pem;                 /* PEM CA cert for HTTPS; NULL skips verification */
    int         timeout_ms;               /* default timeout; BLUSYS_TIMEOUT_FOREVER to block */
    size_t      max_response_body_size;   /* hard cap on buffered body bytes; 0 = unlimited */
} blusys_http_client_config_t;

typedef struct {
    blusys_http_method_t        method;
    const char                 *path;     /* optional path/query override; NULL = use config URL */
    const blusys_http_header_t *headers;  /* NULL-terminated array, or NULL */
    const void                 *body;
    size_t                      body_size;
    int                         timeout_ms; /* per-request override; 0 = use handle default */
} blusys_http_request_t;

/* Response — body is heap-allocated; the caller must free() it when done. */
typedef struct {
    int    status_code;
    void  *body;           /* null-terminated for text convenience; caller owns this pointer */
    size_t body_size;      /* bytes written (excludes the null terminator) */
    bool   body_truncated; /* true if max_response_body_size was reached */
} blusys_http_response_t;

/* Initialise an HTTP client for the given base URL.
   Returns BLUSYS_ERR_INVALID_ARG if config or out_client is NULL or config->url is NULL.
   Returns BLUSYS_ERR_NO_MEM on allocation failure.
   Returns BLUSYS_ERR_NOT_SUPPORTED on targets without WiFi. */
blusys_err_t blusys_http_client_open(const blusys_http_client_config_t *config,
                                     blusys_http_client_t **out_client);

/* Release all resources.  After this call the handle is invalid. */
blusys_err_t blusys_http_client_close(blusys_http_client_t *client);

/* Perform a synchronous HTTP request.
   Blocks until the response is fully received, a network error occurs, or the timeout expires.
   On BLUSYS_OK, out_response->body is heap-allocated and must be freed by the caller.
   HTTP 4xx/5xx status codes are NOT translated to errors — check out_response->status_code.
   Returns BLUSYS_ERR_INVALID_STATE if another request is already in progress on this handle. */
blusys_err_t blusys_http_client_request(blusys_http_client_t        *client,
                                        const blusys_http_request_t *request,
                                        blusys_http_response_t      *out_response);

/* Convenience wrapper: GET request using the handle's default timeout.
   Pass NULL for path to use the URL set at open time. */
blusys_err_t blusys_http_client_get(blusys_http_client_t   *client,
                                    const char             *path,
                                    blusys_http_response_t *out_response);

/* Convenience wrapper: POST request with an optional Content-Type header.
   Pass NULL for content_type to omit the header.
   Pass NULL for path to use the URL set at open time. */
blusys_err_t blusys_http_client_post(blusys_http_client_t   *client,
                                     const char             *path,
                                     const char             *content_type,
                                     const void             *body,
                                     size_t                  body_size,
                                     blusys_http_response_t *out_response);

#ifdef __cplusplus
}
#endif

#endif
