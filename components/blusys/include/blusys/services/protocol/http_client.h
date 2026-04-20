/**
 * @file http_client.h
 * @brief Synchronous HTTP/HTTPS client.
 *
 * Issues one-shot requests against a base URL and returns a heap-allocated
 * response body the caller must `free()`. HTTP 4xx/5xx status codes are not
 * translated to errors — check `response.status_code`. See
 * docs/services/http.md.
 */

#ifndef BLUSYS_HTTP_CLIENT_H
#define BLUSYS_HTTP_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque HTTP client handle. */
typedef struct blusys_http_client blusys_http_client_t;

/** @brief HTTP request method. */
typedef enum {
    BLUSYS_HTTP_METHOD_GET    = 0,   /**< GET. */
    BLUSYS_HTTP_METHOD_POST,         /**< POST. */
    BLUSYS_HTTP_METHOD_PUT,          /**< PUT. */
    BLUSYS_HTTP_METHOD_PATCH,        /**< PATCH. */
    BLUSYS_HTTP_METHOD_DELETE,       /**< DELETE. */
    BLUSYS_HTTP_METHOD_HEAD,         /**< HEAD. */
} blusys_http_method_t;

/**
 * @brief Null-terminated name/value pair for request headers.
 *
 * Terminate the array with `{ NULL, NULL }`.
 */
typedef struct {
    const char *name;   /**< Header name. */
    const char *value;  /**< Header value. */
} blusys_http_header_t;

/** @brief Configuration for ::blusys_http_client_open. */
typedef struct {
    const char *url;                     /**< Required base URL; copied internally at open time. */
    const char *cert_pem;                /**< PEM CA cert for HTTPS; NULL skips verification. */
    int         timeout_ms;              /**< Default timeout; use `BLUSYS_TIMEOUT_FOREVER` to block. */
    size_t      max_response_body_size;  /**< Hard cap on buffered body bytes; `0` = unlimited. */
} blusys_http_client_config_t;

/** @brief Per-request parameters passed to ::blusys_http_client_request. */
typedef struct {
    blusys_http_method_t        method;     /**< HTTP method. */
    const char                 *path;       /**< Optional path/query override; NULL uses the config URL. */
    const blusys_http_header_t *headers;    /**< Optional NULL-terminated header array; may be NULL. */
    const void                 *body;       /**< Optional request body. */
    size_t                      body_size;  /**< Bytes in @ref body. */
    int                         timeout_ms; /**< Per-request override; `0` uses the handle default. */
} blusys_http_request_t;

/**
 * @brief HTTP response.
 *
 * The @ref body pointer is heap-allocated and owned by the caller.
 */
typedef struct {
    int    status_code;     /**< HTTP status code. */
    void  *body;            /**< Null-terminated body buffer; caller must `free()`. */
    size_t body_size;       /**< Bytes written (excludes the null terminator). */
    bool   body_truncated;  /**< `true` if `max_response_body_size` was reached. */
} blusys_http_response_t;

/**
 * @brief Initialise an HTTP client for the given base URL.
 * @param config      Configuration.
 * @param out_client  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_http_client_open(const blusys_http_client_config_t *config,
                                     blusys_http_client_t **out_client);

/**
 * @brief Release all resources. The handle is invalid after this call.
 * @param client  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p client is NULL.
 */
blusys_err_t blusys_http_client_close(blusys_http_client_t *client);

/**
 * @brief Perform a synchronous HTTP request.
 *
 * Blocks until the response is fully received, a network error occurs, or
 * the timeout expires. On `BLUSYS_OK`, `out_response->body` is
 * heap-allocated and must be freed by the caller. HTTP 4xx/5xx status codes
 * are not translated to errors — check `out_response->status_code`.
 *
 * @param client        Handle.
 * @param request       Request parameters.
 * @param out_response  Response (body is heap-allocated).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE`
 *         if another request is already in progress on this handle, or a
 *         translated network error.
 */
blusys_err_t blusys_http_client_request(blusys_http_client_t        *client,
                                        const blusys_http_request_t *request,
                                        blusys_http_response_t      *out_response);

/**
 * @brief Convenience wrapper: GET request using the handle's default timeout.
 * @param client        Handle.
 * @param path          Optional path override; NULL uses the URL set at open time.
 * @param out_response  Response.
 * @return See ::blusys_http_client_request.
 */
blusys_err_t blusys_http_client_get(blusys_http_client_t   *client,
                                    const char             *path,
                                    blusys_http_response_t *out_response);

/**
 * @brief Convenience wrapper: POST request with an optional Content-Type.
 * @param client        Handle.
 * @param path          Optional path override; NULL uses the URL set at open time.
 * @param content_type  Optional MIME type; NULL omits the header.
 * @param body          Request body.
 * @param body_size     Bytes in @p body.
 * @param out_response  Response.
 * @return See ::blusys_http_client_request.
 */
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
