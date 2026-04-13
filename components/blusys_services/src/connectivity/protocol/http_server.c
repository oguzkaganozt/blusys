#include "blusys/connectivity/protocol/http_server.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "esp_http_server.h"

#include "blusys/internal/blusys_esp_err.h"

struct blusys_http_server {
    httpd_handle_t                esp_handle;
    volatile bool                 running;
    blusys_http_server_route_t   *routes;       /* owned copy of the user's route array */
    size_t                        route_count;
};

struct blusys_http_server_req {
    httpd_req_t *esp_req;
};

static httpd_method_t to_esp_method(blusys_http_method_t m)
{
    switch (m) {
    case BLUSYS_HTTP_METHOD_POST:   return HTTP_POST;
    case BLUSYS_HTTP_METHOD_PUT:    return HTTP_PUT;
    case BLUSYS_HTTP_METHOD_PATCH:  return HTTP_PATCH;
    case BLUSYS_HTTP_METHOD_DELETE: return HTTP_DELETE;
    case BLUSYS_HTTP_METHOD_HEAD:   return HTTP_HEAD;
    default:                        return HTTP_GET;
    }
}

static const char *status_string(int code)
{
    switch (code) {
    case 200: return "200 OK";
    case 201: return "201 Created";
    case 204: return "204 No Content";
    case 400: return "400 Bad Request";
    case 401: return "401 Unauthorized";
    case 403: return "403 Forbidden";
    case 404: return "404 Not Found";
    case 500: return "500 Internal Server Error";
    default:  return "500 Internal Server Error";
    }
}

static esp_err_t route_wrapper(httpd_req_t *req)
{
    const blusys_http_server_route_t *route = req->user_ctx;
    blusys_http_server_req_t blusys_req = { .esp_req = req };
    blusys_err_t err = route->handler(&blusys_req, route->user_ctx);
    if (err != BLUSYS_OK) {
        /* Handler did not send a response — send 500 so the client gets a reply. */
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, NULL, 0);
    }
    return ESP_OK;
}

blusys_err_t blusys_http_server_open(const blusys_http_server_config_t *config,
                                      blusys_http_server_t             **out_handle)
{
    if ((config == NULL) || (out_handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if ((config->route_count > 0) && (config->routes == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_http_server_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    httpd_config_t esp_cfg = HTTPD_DEFAULT_CONFIG();
    if (config->port != 0) {
        esp_cfg.server_port = config->port;
    }
    if (config->max_open_sockets != 0) {
        esp_cfg.max_open_sockets = (int)config->max_open_sockets;
    }
    esp_cfg.max_uri_handlers = (int)config->route_count;

    /* Deep-copy the routes array so callers need not keep it alive */
    if (config->route_count > 0) {
        h->routes = calloc(config->route_count, sizeof(blusys_http_server_route_t));
        if (h->routes == NULL) {
            free(h);
            return BLUSYS_ERR_NO_MEM;
        }
        memcpy(h->routes, config->routes,
               config->route_count * sizeof(blusys_http_server_route_t));
        h->route_count = config->route_count;
    }

    esp_err_t esp_err = httpd_start(&h->esp_handle, &esp_cfg);
    if (esp_err != ESP_OK) {
        free(h->routes);
        free(h);
        return blusys_translate_esp_err(esp_err);
    }

    for (size_t i = 0; i < h->route_count; i++) {
        httpd_uri_t uri_cfg = {
            .uri      = h->routes[i].uri,
            .method   = to_esp_method(h->routes[i].method),
            .handler  = route_wrapper,
            .user_ctx = (void *)&h->routes[i],
        };
        esp_err = httpd_register_uri_handler(h->esp_handle, &uri_cfg);
        if (esp_err != ESP_OK) {
            httpd_stop(h->esp_handle);
            free(h->routes);
            free(h);
            return blusys_translate_esp_err(esp_err);
        }
    }

    h->running  = true;
    *out_handle = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_http_server_close(blusys_http_server_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err_t esp_err = httpd_stop(handle->esp_handle);
    handle->running   = false;
    free(handle->routes);
    free(handle);
    return blusys_translate_esp_err(esp_err);
}

bool blusys_http_server_is_running(blusys_http_server_t *handle)
{
    if (handle == NULL) {
        return false;
    }
    return handle->running;
}

const char *blusys_http_server_req_uri(blusys_http_server_req_t *req)
{
    if (req == NULL) {
        return NULL;
    }
    return req->esp_req->uri;
}

size_t blusys_http_server_req_content_len(blusys_http_server_req_t *req)
{
    if (req == NULL) {
        return 0;
    }
    return (size_t)req->esp_req->content_len;
}

blusys_err_t blusys_http_server_req_recv(blusys_http_server_req_t *req,
                                          uint8_t                  *buf,
                                          size_t                    buf_len,
                                          size_t                   *out_received)
{
    if ((req == NULL) || (buf == NULL) || (out_received == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    int ret = httpd_req_recv(req->esp_req, (char *)buf, buf_len);
    if (ret < 0) {
        *out_received = 0;
        return (ret == HTTPD_SOCK_ERR_TIMEOUT) ? BLUSYS_ERR_TIMEOUT : BLUSYS_ERR_IO;
    }

    *out_received = (size_t)ret;
    return BLUSYS_OK;
}

blusys_err_t blusys_http_server_req_get_header(blusys_http_server_req_t *req,
                                                const char               *name,
                                                char                     *out_buf,
                                                size_t                    buf_len)
{
    if ((req == NULL) || (name == NULL) || (out_buf == NULL) || (buf_len == 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err_t esp_err = httpd_req_get_hdr_value_str(req->esp_req, name, out_buf, buf_len);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_http_server_resp_send(blusys_http_server_req_t *req,
                                           int                       status_code,
                                           const char               *content_type,
                                           const char               *body,
                                           int                       body_len)
{
    if (req == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    httpd_resp_set_status(req->esp_req, status_string(status_code));

    if (content_type != NULL) {
        httpd_resp_set_type(req->esp_req, content_type);
    }

    esp_err_t esp_err = httpd_resp_send(req->esp_req, body, (ssize_t)body_len);
    return blusys_translate_esp_err(esp_err);
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_http_server_open(const blusys_http_server_config_t *config,
                                      blusys_http_server_t             **out_handle)
{
    (void) config;
    (void) out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_server_close(blusys_http_server_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_http_server_is_running(blusys_http_server_t *handle)
{
    (void) handle;
    return false;
}

const char *blusys_http_server_req_uri(blusys_http_server_req_t *req)
{
    (void) req;
    return NULL;
}

size_t blusys_http_server_req_content_len(blusys_http_server_req_t *req)
{
    (void) req;
    return 0;
}

blusys_err_t blusys_http_server_req_recv(blusys_http_server_req_t *req,
                                          uint8_t                  *buf,
                                          size_t                    buf_len,
                                          size_t                   *out_received)
{
    (void) req;
    (void) buf;
    (void) buf_len;
    (void) out_received;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_server_req_get_header(blusys_http_server_req_t *req,
                                                const char               *name,
                                                char                     *out_buf,
                                                size_t                    buf_len)
{
    (void) req;
    (void) name;
    (void) out_buf;
    (void) buf_len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_server_resp_send(blusys_http_server_req_t *req,
                                           int                       status_code,
                                           const char               *content_type,
                                           const char               *body,
                                           int                       body_len)
{
    (void) req;
    (void) status_code;
    (void) content_type;
    (void) body;
    (void) body_len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_WIFI_SUPPORTED */
