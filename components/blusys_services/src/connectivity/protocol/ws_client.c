#include "blusys/connectivity/protocol/ws_client.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_transport_ssl.h"
#include "esp_transport_ws.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_timeout.h"

#define WS_RECV_BUF_SIZE   1024
#define WS_RECV_TASK_STACK 4096
#define WS_RECV_TASK_PRIO  (tskIDLE_PRIORITY + 2)
/* Poll interval for the receive loop — short enough to detect h->running = false
   promptly after disconnect() is called, long enough to avoid busy-looping. */
#define WS_RECV_POLL_MS    500

struct blusys_ws_client {
    esp_transport_handle_t    tcp_transport;
    esp_transport_handle_t    ws_transport;
    blusys_lock_t             lock;
    SemaphoreHandle_t         task_done_sem;
    TaskHandle_t              recv_task;
    blusys_ws_message_cb_t    message_cb;
    blusys_ws_disconnect_cb_t disconnect_cb;
    void                     *user_ctx;
    int                       timeout_ms;
    volatile bool             connected;
    volatile bool             running;
    char                      host[128];
    int                       port;
    char                      path[128];
    bool                      use_tls;
    const char               *server_cert_pem; /* pointer to caller-owned PEM string */
    const char               *subprotocol;     /* pointer to caller-owned string */
};

/* ---- URL parsing --------------------------------------------------------- */

static blusys_err_t parse_ws_url(const char *url,
                                  char *host, size_t host_len,
                                  int  *port,
                                  char *path, size_t path_len,
                                  bool *use_tls)
{
    const char *p = url;

    if (strncmp(p, "wss://", 6) == 0) {
        *use_tls = true;
        *port    = 443;
        p       += 6;
    } else if (strncmp(p, "ws://", 5) == 0) {
        *use_tls = false;
        *port    = 80;
        p       += 5;
    } else {
        return BLUSYS_ERR_INVALID_ARG;
    }

    const char *host_start = p;
    const char *port_colon = NULL;
    const char *path_slash = NULL;

    while (*p != '\0' && *p != ':' && *p != '/') {
        p++;
    }
    if (*p == ':') {
        port_colon = p;
        p++;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = (*port) * 10 + (*p - '0');
            p++;
        }
        if (*port > 65535) {
            return BLUSYS_ERR_INVALID_ARG;
        }
    }
    if (*p == '/') {
        path_slash = p;
    }

    const char *host_end = port_colon ? port_colon : (path_slash ? path_slash : p);
    size_t hlen = (size_t)(host_end - host_start);
    if (hlen == 0 || hlen >= host_len) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    memcpy(host, host_start, hlen);
    host[hlen] = '\0';

    if (path_slash != NULL) {
        size_t plen = strlen(path_slash);
        if (plen >= path_len) {
            return BLUSYS_ERR_INVALID_ARG;
        }
        memcpy(path, path_slash, plen + 1);
    } else {
        path[0] = '/';
        path[1] = '\0';
    }

    return BLUSYS_OK;
}

/* ---- Transport helpers --------------------------------------------------- */

static blusys_err_t create_transports(blusys_ws_client_t *h)
{
    if (h->use_tls) {
        h->tcp_transport = esp_transport_ssl_init();
        if (h->tcp_transport == NULL) {
            return BLUSYS_ERR_NO_MEM;
        }
        if (h->server_cert_pem != NULL) {
            esp_transport_ssl_set_cert_data(h->tcp_transport,
                                            h->server_cert_pem,
                                            (int)strlen(h->server_cert_pem));
        } else {
            /* No CA cert — skip CN verification for wss:// */
            esp_transport_ssl_skip_common_name_check(h->tcp_transport);
        }
    } else {
        h->tcp_transport = esp_transport_tcp_init();
        if (h->tcp_transport == NULL) {
            return BLUSYS_ERR_NO_MEM;
        }
    }

    h->ws_transport = esp_transport_ws_init(h->tcp_transport);
    if (h->ws_transport == NULL) {
        esp_transport_destroy(h->tcp_transport);
        h->tcp_transport = NULL;
        return BLUSYS_ERR_NO_MEM;
    }

    esp_transport_ws_set_path(h->ws_transport, h->path);

    if (h->subprotocol != NULL) {
        esp_transport_ws_set_subprotocol(h->ws_transport, h->subprotocol);
    }

    return BLUSYS_OK;
}

static void destroy_transports(blusys_ws_client_t *h)
{
    if (h->ws_transport != NULL) {
        esp_transport_destroy(h->ws_transport);
        h->ws_transport = NULL;
    }
    if (h->tcp_transport != NULL) {
        esp_transport_destroy(h->tcp_transport);
        h->tcp_transport = NULL;
    }
}

/* ---- Receive task -------------------------------------------------------- */

static void ws_recv_task(void *arg)
{
    blusys_ws_client_t *h = arg;
    char buf[WS_RECV_BUF_SIZE];
    bool peer_disconnected = false;

    while (h->running) {
        int len = esp_transport_read(h->ws_transport, buf, sizeof(buf), WS_RECV_POLL_MS);

        if (!h->running) {
            break;
        }

        if (len > 0) {
            ws_transport_opcodes_t opcode = esp_transport_ws_get_read_opcode(h->ws_transport);
            switch (opcode) {
            case WS_TRANSPORT_OPCODES_TEXT:
                h->message_cb(BLUSYS_WS_MSG_TEXT,
                              (const uint8_t *)buf, (size_t)len,
                              h->user_ctx);
                break;
            case WS_TRANSPORT_OPCODES_BINARY:
                h->message_cb(BLUSYS_WS_MSG_BINARY,
                              (const uint8_t *)buf, (size_t)len,
                              h->user_ctx);
                break;
            case WS_TRANSPORT_OPCODES_CLOSE:
                peer_disconnected = true;
                h->running        = false;
                break;
            default:
                break;
            }
        } else if (len == 0) {
            /* poll timeout — loop and re-check h->running */
        } else {
            /* transport error or EOF — treat as peer disconnect if h->running
               was still true (i.e. not a user-initiated close) */
            peer_disconnected = true;
            h->running        = false;
        }
    }

    /* Only fire disconnect_cb for peer-initiated disconnects (server CLOSE
       frame or network error).  User-initiated disconnect() sets h->running =
       false before closing the transport, so peer_disconnected stays false. */
    if (peer_disconnected && h->disconnect_cb != NULL) {
        h->disconnect_cb(h->user_ctx);
    }

    xSemaphoreGive(h->task_done_sem);
    vTaskDelete(NULL);
}

/* ---- Public API ---------------------------------------------------------- */

blusys_err_t blusys_ws_client_open(const blusys_ws_client_config_t *config,
                                    blusys_ws_client_t             **out_handle)
{
    if ((config == NULL) || (config->url == NULL) ||
        (config->message_cb == NULL) || (out_handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (!blusys_timeout_ms_is_valid(config->timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_ws_client_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = parse_ws_url(config->url,
                                     h->host, sizeof(h->host),
                                     &h->port,
                                     h->path, sizeof(h->path),
                                     &h->use_tls);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    h->task_done_sem = xSemaphoreCreateBinary();
    if (h->task_done_sem == NULL) {
        blusys_lock_deinit(&h->lock);
        free(h);
        return BLUSYS_ERR_NO_MEM;
    }

    h->message_cb     = config->message_cb;
    h->disconnect_cb  = config->disconnect_cb;
    h->user_ctx       = config->user_ctx;
    h->timeout_ms     = config->timeout_ms;
    h->server_cert_pem = config->server_cert_pem;
    h->subprotocol    = config->subprotocol;

    *out_handle = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_ws_client_close(blusys_ws_client_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Disconnect if connected — unconditional; disconnect() returns OK immediately
       if not connected, so no need to check handle->connected without the lock. */
    blusys_ws_client_disconnect(handle);

    blusys_lock_deinit(&handle->lock);
    vSemaphoreDelete(handle->task_done_sem);
    free(handle);
    return BLUSYS_OK;
}

blusys_err_t blusys_ws_client_connect(blusys_ws_client_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = create_transports(handle);

    blusys_lock_give(&handle->lock);

    if (err != BLUSYS_OK) {
        return err;
    }

    /* Blocking TCP + WebSocket handshake — lock NOT held (CLAUDE.md rule). */
    int timeout = (handle->timeout_ms == BLUSYS_TIMEOUT_FOREVER) ? -1 : handle->timeout_ms;
    int ret = esp_transport_connect(handle->ws_transport, handle->host, handle->port, timeout);
    if (ret != 0) {
        destroy_transports(handle);
        return BLUSYS_ERR_IO;
    }

    /* Re-take lock to update state and spawn receive task. */
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        destroy_transports(handle);
        return err;
    }

    handle->connected = true;
    handle->running   = true;

    BaseType_t created = xTaskCreate(ws_recv_task, "ws_recv",
                                      WS_RECV_TASK_STACK, handle,
                                      WS_RECV_TASK_PRIO, &handle->recv_task);

    if (created != pdPASS) {
        /* Reset state while still holding the lock so no concurrent caller
           can observe connected=true on a handle with no receive task. */
        handle->connected = false;
        handle->running   = false;
        blusys_lock_give(&handle->lock);
        esp_transport_close(handle->ws_transport);
        destroy_transports(handle);
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_lock_give(&handle->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_ws_client_disconnect(blusys_ws_client_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_OK;
    }

    /* Signal the receive task to stop.  Also mark as not connected so that any
       concurrent send() that captured ws_transport but hasn't called send_raw
       yet will return IO error rather than operating on a closing transport. */
    handle->running   = false;
    handle->connected = false;

    blusys_lock_give(&handle->lock);

    /* Close the transport — interrupts any pending esp_transport_read(). */
    esp_transport_close(handle->ws_transport);

    /* Wait for the receive task to acknowledge shutdown (signals task_done_sem
       just before calling vTaskDelete). */
    xSemaphoreTake(handle->task_done_sem, portMAX_DELAY);

    destroy_transports(handle);

    return BLUSYS_OK;
}

blusys_err_t blusys_ws_client_send_text(blusys_ws_client_t *handle, const char *text)
{
    if ((handle == NULL) || (text == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    /* Capture the transport under the lock, then release before the blocking
       send — consistent with the http_client pattern (lock taken with timeout 0
       to serialise; I/O performed without holding the lock).
       Callers must not call send() concurrently with disconnect() on the same
       handle; disconnect() sets connected = false under the lock first, so any
       send() arriving after that will see INVALID_STATE without touching the
       transport. */
    esp_transport_handle_t t = handle->ws_transport;
    int timeout = (handle->timeout_ms == BLUSYS_TIMEOUT_FOREVER) ? -1 : handle->timeout_ms;

    blusys_lock_give(&handle->lock);

    int sent = esp_transport_ws_send_raw(t, WS_TRANSPORT_OPCODES_TEXT,
                                          text, (int)strlen(text), timeout);
    return (sent >= 0) ? BLUSYS_OK : BLUSYS_ERR_IO;
}

blusys_err_t blusys_ws_client_send(blusys_ws_client_t *handle,
                                    const uint8_t      *data,
                                    size_t              len)
{
    if ((handle == NULL) || (data == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_transport_handle_t t = handle->ws_transport;
    int timeout = (handle->timeout_ms == BLUSYS_TIMEOUT_FOREVER) ? -1 : handle->timeout_ms;

    blusys_lock_give(&handle->lock);

    int sent = esp_transport_ws_send_raw(t, WS_TRANSPORT_OPCODES_BINARY,
                                          (const char *)data, (int)len, timeout);
    return (sent >= 0) ? BLUSYS_OK : BLUSYS_ERR_IO;
}

bool blusys_ws_client_is_connected(blusys_ws_client_t *handle)
{
    if (handle == NULL) {
        return false;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return false;
    }

    bool connected = handle->connected;

    blusys_lock_give(&handle->lock);
    return connected;
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_ws_client_open(const blusys_ws_client_config_t *config,
                                    blusys_ws_client_t             **out_handle)
{
    (void) config;
    (void) out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ws_client_close(blusys_ws_client_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ws_client_connect(blusys_ws_client_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ws_client_disconnect(blusys_ws_client_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ws_client_send_text(blusys_ws_client_t *handle, const char *text)
{
    (void) handle;
    (void) text;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ws_client_send(blusys_ws_client_t *handle,
                                    const uint8_t      *data,
                                    size_t              len)
{
    (void) handle;
    (void) data;
    (void) len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_ws_client_is_connected(blusys_ws_client_t *handle)
{
    (void) handle;
    return false;
}

#endif /* SOC_WIFI_SUPPORTED */
