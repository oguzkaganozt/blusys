#include "blusys/connectivity/protocol/http_client.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "esp_http_client.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_timeout.h"

#define BODY_BUF_INITIAL_CAP 512u

struct blusys_http_client {
    esp_http_client_handle_t  esp_handle;
    blusys_lock_t             lock;
    int                       default_timeout_ms;
    size_t                    max_response_body_size;
    /* per-request fields, reset before every perform() */
    uint8_t                  *body_buf;
    size_t                    body_buf_len;
    size_t                    body_buf_cap;
    bool                      body_truncated;
    blusys_err_t              cb_error;
};

static esp_http_client_method_t translate_method(blusys_http_method_t m)
{
    switch (m) {
    case BLUSYS_HTTP_METHOD_POST:   return HTTP_METHOD_POST;
    case BLUSYS_HTTP_METHOD_PUT:    return HTTP_METHOD_PUT;
    case BLUSYS_HTTP_METHOD_PATCH:  return HTTP_METHOD_PATCH;
    case BLUSYS_HTTP_METHOD_DELETE: return HTTP_METHOD_DELETE;
    case BLUSYS_HTTP_METHOD_HEAD:   return HTTP_METHOD_HEAD;
    case BLUSYS_HTTP_METHOD_GET:
    default:                        return HTTP_METHOD_GET;
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    blusys_http_client_t *h = evt->user_data;

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        h->cb_error = BLUSYS_ERR_IO;
        break;

    case HTTP_EVENT_ON_DATA:
        if (h->body_truncated || h->cb_error != BLUSYS_OK) {
            break;
        }

        size_t incoming = (size_t)evt->data_len;

        if (h->max_response_body_size > 0) {
            size_t remaining = h->max_response_body_size - h->body_buf_len;
            if (incoming > remaining) {
                incoming = remaining;
                h->body_truncated = true;
            }
        }

        if (incoming == 0) {
            break;
        }

        if (h->body_buf_len + incoming + 1 > h->body_buf_cap) {
            size_t new_cap = h->body_buf_cap == 0 ? BODY_BUF_INITIAL_CAP : h->body_buf_cap * 2;
            while (new_cap < h->body_buf_len + incoming + 1) {
                new_cap *= 2;
            }
            uint8_t *nb = realloc(h->body_buf, new_cap);
            if (nb == NULL) {
                h->cb_error = BLUSYS_ERR_NO_MEM;
                h->body_truncated = true;
                break;
            }
            h->body_buf = nb;
            h->body_buf_cap = new_cap;
        }

        memcpy(h->body_buf + h->body_buf_len, evt->data, incoming);
        h->body_buf_len += incoming;
        h->body_buf[h->body_buf_len] = '\0';
        break;

    default:
        break;
    }

    return ESP_OK;
}

blusys_err_t blusys_http_client_open(const blusys_http_client_config_t *config,
                                     blusys_http_client_t **out_client)
{
    if ((config == NULL) || (config->url == NULL) || (out_client == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_http_client_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    h->default_timeout_ms      = config->timeout_ms;
    h->max_response_body_size  = config->max_response_body_size;

    /* BLUSYS_TIMEOUT_FOREVER (-1) maps to 0 in esp_http_client (use driver default) */
    int esp_timeout = (config->timeout_ms < 0) ? 0 : config->timeout_ms;

    esp_http_client_config_t esp_cfg = {
        .url              = config->url,
        .cert_pem         = config->cert_pem,
        .event_handler    = http_event_handler,
        .user_data        = h,
        .timeout_ms       = esp_timeout,
    };

    h->esp_handle = esp_http_client_init(&esp_cfg);
    if (h->esp_handle == NULL) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_init;
    }

    *out_client = h;
    return BLUSYS_OK;

fail_init:
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_http_client_close(blusys_http_client_t *client)
{
    if (client == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&client->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_http_client_cleanup(client->esp_handle);
    free(client->body_buf);

    blusys_lock_give(&client->lock);
    blusys_lock_deinit(&client->lock);
    free(client);
    return BLUSYS_OK;
}

blusys_err_t blusys_http_client_request(blusys_http_client_t        *client,
                                        const blusys_http_request_t *request,
                                        blusys_http_response_t      *out_response)
{
    if ((client == NULL) || (request == NULL) || (out_response == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (!blusys_timeout_ms_is_valid(request->timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Non-blocking lock take — reject concurrent requests */
    blusys_err_t err = blusys_lock_take(&client->lock, 0);
    if (err != BLUSYS_OK) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    free(client->body_buf);
    client->body_buf       = NULL;
    client->body_buf_len   = 0;
    client->body_buf_cap   = 0;
    client->body_truncated = false;
    client->cb_error       = BLUSYS_OK;

    if (request->path != NULL) {
        esp_http_client_set_url(client->esp_handle, request->path);
    }

    esp_http_client_set_method(client->esp_handle, translate_method(request->method));

    if (request->headers != NULL) {
        for (const blusys_http_header_t *hdr = request->headers;
             hdr->name != NULL;
             hdr++) {
            esp_http_client_set_header(client->esp_handle, hdr->name, hdr->value);
        }
    }

    if ((request->body != NULL) && (request->body_size > 0)) {
        esp_http_client_set_post_field(client->esp_handle,
                                       request->body,
                                       (int)request->body_size);
    } else {
        esp_http_client_set_post_field(client->esp_handle, NULL, 0);
    }

    /* Apply timeout override — 0 means "use handle default" */
    int effective_timeout = (request->timeout_ms != 0) ? request->timeout_ms
                                                        : client->default_timeout_ms;
    int esp_timeout = (effective_timeout < 0) ? 0 : effective_timeout;
    esp_http_client_set_timeout_ms(client->esp_handle, esp_timeout);

    esp_err_t esp_err = esp_http_client_perform(client->esp_handle);

    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto cleanup;
    }

    if (client->cb_error != BLUSYS_OK) {
        err = client->cb_error;
        goto cleanup;
    }

    /* Transfer body buffer ownership to caller */
    out_response->status_code   = esp_http_client_get_status_code(client->esp_handle);
    out_response->body          = client->body_buf;
    out_response->body_size     = client->body_buf_len;
    out_response->body_truncated = client->body_truncated;

    client->body_buf = NULL; /* handle no longer owns it */

    blusys_lock_give(&client->lock);
    return BLUSYS_OK;

cleanup:
    free(client->body_buf);
    client->body_buf = NULL;
    out_response->body           = NULL;
    out_response->body_size      = 0;
    out_response->body_truncated = false;
    out_response->status_code    = 0;
    blusys_lock_give(&client->lock);
    return err;
}

blusys_err_t blusys_http_client_get(blusys_http_client_t   *client,
                                    const char             *path,
                                    blusys_http_response_t *out_response)
{
    blusys_http_request_t req = {
        .method     = BLUSYS_HTTP_METHOD_GET,
        .path       = path,
        .headers    = NULL,
        .body       = NULL,
        .body_size  = 0,
        .timeout_ms = 0,
    };
    return blusys_http_client_request(client, &req, out_response);
}

blusys_err_t blusys_http_client_post(blusys_http_client_t   *client,
                                     const char             *path,
                                     const char             *content_type,
                                     const void             *body,
                                     size_t                  body_size,
                                     blusys_http_response_t *out_response)
{
    blusys_http_header_t headers[2] = {
        { "Content-Type", content_type },
        { NULL, NULL },
    };

    blusys_http_request_t req = {
        .method     = BLUSYS_HTTP_METHOD_POST,
        .path       = path,
        .headers    = (content_type != NULL) ? headers : NULL,
        .body       = body,
        .body_size  = body_size,
        .timeout_ms = 0,
    };
    return blusys_http_client_request(client, &req, out_response);
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_http_client_open(const blusys_http_client_config_t *config,
                                     blusys_http_client_t **out_client)
{
    (void) config;
    (void) out_client;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_client_close(blusys_http_client_t *client)
{
    (void) client;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_client_request(blusys_http_client_t        *client,
                                        const blusys_http_request_t *request,
                                        blusys_http_response_t      *out_response)
{
    (void) client;
    (void) request;
    (void) out_response;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_client_get(blusys_http_client_t   *client,
                                    const char             *path,
                                    blusys_http_response_t *out_response)
{
    (void) client;
    (void) path;
    (void) out_response;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_http_client_post(blusys_http_client_t   *client,
                                     const char             *path,
                                     const char             *content_type,
                                     const void             *body,
                                     size_t                  body_size,
                                     blusys_http_response_t *out_response)
{
    (void) client;
    (void) path;
    (void) content_type;
    (void) body;
    (void) body_size;
    (void) out_response;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_WIFI_SUPPORTED */
