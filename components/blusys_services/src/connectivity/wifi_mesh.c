#include "blusys/connectivity/wifi_mesh.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "esp_event.h"
#include "esp_mesh.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_nvs_init.h"

#define MESH_DEFAULT_MAX_LAYER       6
#define MESH_DEFAULT_MAX_CONNECTIONS 6

struct blusys_wifi_mesh {
    blusys_lock_t                     lock;
    esp_netif_t                      *sta_netif;
    esp_netif_t                      *ap_netif;
    esp_event_handler_instance_t      mesh_handler;
    esp_event_handler_instance_t      ip_handler;
    blusys_wifi_mesh_event_cb_t       on_event;
    void                             *user_ctx;
    volatile bool                     closing;
};

/* s_handle enforces the singleton constraint — only one open instance at a time. */
static portMUX_TYPE          s_mesh_init_lock = portMUX_INITIALIZER_UNLOCKED;
static blusys_lock_t         s_mesh_global_lock;
static bool                  s_mesh_global_lock_inited;
static blusys_wifi_mesh_t   *s_handle;

static blusys_err_t ensure_global_lock(void)
{
    if (s_mesh_global_lock_inited) {
        return BLUSYS_OK;
    }
    blusys_lock_t new_lock;
    blusys_err_t err = blusys_lock_init(&new_lock);
    if (err != BLUSYS_OK) {
        return err;
    }
    portENTER_CRITICAL(&s_mesh_init_lock);
    if (!s_mesh_global_lock_inited) {
        s_mesh_global_lock = new_lock;
        s_mesh_global_lock_inited = true;
        portEXIT_CRITICAL(&s_mesh_init_lock);
    } else {
        portEXIT_CRITICAL(&s_mesh_init_lock);
        blusys_lock_deinit(&new_lock);
    }
    return BLUSYS_OK;
}

static void emit_event(blusys_wifi_mesh_t *h, blusys_wifi_mesh_event_t event,
                       const blusys_wifi_mesh_event_info_t *info)
{
    if (h->on_event != NULL) {
        h->on_event(h, event, info, h->user_ctx);
    }
}

static void mesh_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    (void)base;
    blusys_wifi_mesh_t *h = (blusys_wifi_mesh_t *)arg;
    if (h->closing) {
        return;
    }

    blusys_wifi_mesh_event_info_t info;
    memset(&info, 0, sizeof(info));

    switch ((mesh_event_id_t)id) {
        case MESH_EVENT_STARTED:
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_STARTED, NULL);
            break;

        case MESH_EVENT_STOPPED:
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_STOPPED, NULL);
            break;

        case MESH_EVENT_PARENT_CONNECTED: {
            const mesh_event_connected_t *evt = (const mesh_event_connected_t *)data;
            if (evt != NULL) {
                memcpy(info.peer.addr, evt->connected.bssid, 6);
            }
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_PARENT_CONNECTED, &info);
            break;
        }

        case MESH_EVENT_PARENT_DISCONNECTED: {
            const mesh_event_disconnected_t *evt = (const mesh_event_disconnected_t *)data;
            if (evt != NULL) {
                memcpy(info.peer.addr, evt->bssid, 6);
            }
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_PARENT_DISCONNECTED, &info);
            break;
        }

        case MESH_EVENT_CHILD_CONNECTED: {
            const mesh_event_child_connected_t *evt = (const mesh_event_child_connected_t *)data;
            if (evt != NULL) {
                memcpy(info.peer.addr, evt->mac, 6);
            }
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_CHILD_CONNECTED, &info);
            break;
        }

        case MESH_EVENT_CHILD_DISCONNECTED: {
            const mesh_event_child_disconnected_t *evt = (const mesh_event_child_disconnected_t *)data;
            if (evt != NULL) {
                memcpy(info.peer.addr, evt->mac, 6);
            }
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_CHILD_DISCONNECTED, &info);
            break;
        }

        case MESH_EVENT_NO_PARENT_FOUND:
            emit_event(h, BLUSYS_WIFI_MESH_EVENT_NO_PARENT_FOUND, NULL);
            break;

        default:
            break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base,
                              int32_t id, void *data)
{
    (void)base;
    (void)id;
    (void)data;
    blusys_wifi_mesh_t *h = (blusys_wifi_mesh_t *)arg;
    if (h->closing) {
        return;
    }
    emit_event(h, BLUSYS_WIFI_MESH_EVENT_GOT_IP, NULL);
}

blusys_err_t blusys_wifi_mesh_open(const blusys_wifi_mesh_config_t *config,
                                    blusys_wifi_mesh_t **out_handle)
{
    if ((config == NULL) || (out_handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t gerr = ensure_global_lock();
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    gerr = blusys_lock_take(&s_mesh_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    if (s_handle != NULL) {
        blusys_lock_give(&s_mesh_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_wifi_mesh_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_mesh_global_lock);
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_mesh_global_lock);
        free(h);
        return err;
    }

    h->on_event = config->on_event;
    h->user_ctx = config->user_ctx;

    err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        goto fail_nvs;
    }

    esp_err_t esp_err = esp_netif_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_netif_init;
    }

    esp_err = esp_event_loop_create_default();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_event_loop;
    }

    esp_err = esp_netif_create_default_wifi_mesh_netifs(&h->sta_netif, &h->ap_netif);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_netif_create;
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err = esp_wifi_init(&wifi_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_init;
    }

    esp_err = esp_event_handler_instance_register(MESH_EVENT, ESP_EVENT_ANY_ID,
                                                   mesh_event_handler, h,
                                                   &h->mesh_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler_mesh;
    }

    esp_err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   ip_event_handler, h,
                                                   &h->ip_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler_ip;
    }

    esp_err = esp_mesh_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_mesh_init;
    }

    int max_layer = (config->max_layer > 0) ? config->max_layer : MESH_DEFAULT_MAX_LAYER;
    esp_err = esp_mesh_set_max_layer(max_layer);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_mesh_cfg;
    }

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy(cfg.mesh_id.addr, config->mesh_id, 6);
    cfg.channel = config->channel;

    if (config->router_ssid != NULL) {
        cfg.router.ssid_len = (uint8_t)strlen(config->router_ssid);
        memcpy(cfg.router.ssid, config->router_ssid, cfg.router.ssid_len);
        if (config->router_password != NULL) {
            memcpy(cfg.router.password, config->router_password,
                   strlen(config->router_password));
        }
    }

    if (config->password != NULL) {
        memcpy(cfg.mesh_ap.password, config->password, strlen(config->password));
        esp_err = esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_mesh_cfg;
        }
    }

    int max_conn = (config->max_connections > 0)
                 ? config->max_connections
                 : MESH_DEFAULT_MAX_CONNECTIONS;
    cfg.mesh_ap.max_connection = (uint8_t)max_conn;

    esp_err = esp_mesh_set_config(&cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_mesh_cfg;
    }

    /* esp_mesh_start() sets WiFi to APSTA mode and starts it internally;
       do not call esp_wifi_start() before this point. */
    esp_err = esp_mesh_start();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_mesh_start;
    }

    s_handle = h;
    blusys_lock_give(&s_mesh_global_lock);

    *out_handle = h;
    return BLUSYS_OK;

fail_mesh_start:
fail_mesh_cfg:
    esp_mesh_deinit();
fail_mesh_init:
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, h->ip_handler);
fail_handler_ip:
    esp_event_handler_instance_unregister(MESH_EVENT, ESP_EVENT_ANY_ID, h->mesh_handler);
fail_handler_mesh:
    esp_wifi_deinit();
fail_wifi_init:
    esp_netif_destroy(h->sta_netif);
    esp_netif_destroy(h->ap_netif);
fail_netif_create:
    esp_event_loop_delete_default();
fail_event_loop:
    esp_netif_deinit();
fail_netif_init:
fail_nvs:
    blusys_lock_give(&s_mesh_global_lock);
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_wifi_mesh_close(blusys_wifi_mesh_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    handle->closing = true;

    blusys_lock_give(&handle->lock);

    /* Unregister first — blocks until any in-progress dispatch completes,
       so no handler touches the handle after this point. */
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, handle->ip_handler);
    esp_event_handler_instance_unregister(MESH_EVENT, ESP_EVENT_ANY_ID, handle->mesh_handler);

    esp_mesh_stop();
    esp_mesh_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();

    esp_netif_destroy(handle->sta_netif);
    esp_netif_destroy(handle->ap_netif);
    esp_event_loop_delete_default();
    esp_netif_deinit();

    err = blusys_lock_take(&s_mesh_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    s_handle = NULL;
    blusys_lock_give(&s_mesh_global_lock);

    blusys_lock_deinit(&handle->lock);
    free(handle);
    return BLUSYS_OK;
}

blusys_err_t blusys_wifi_mesh_send(blusys_wifi_mesh_t *handle,
                                    const blusys_wifi_mesh_addr_t *dst,
                                    const void *data, size_t len)
{
    if ((handle == NULL) || (dst == NULL) || (data == NULL) || (len == 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (len > UINT16_MAX) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    mesh_addr_t mdst;
    memcpy(mdst.addr, dst->addr, 6);

    mesh_data_t mdata = {
        .data  = (uint8_t *)data,
        .size  = (uint16_t)len,
        .proto = MESH_PROTO_BIN,
        .tos   = MESH_TOS_P2P,
    };

    esp_err_t esp_err = esp_mesh_send(&mdst, &mdata, 0, NULL, 0);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_wifi_mesh_recv(blusys_wifi_mesh_t *handle,
                                    blusys_wifi_mesh_addr_t *src,
                                    void *buf, size_t *len,
                                    int timeout_ms)
{
    if ((handle == NULL) || (buf == NULL) || (len == NULL) || (*len == 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (*len > UINT16_MAX) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    mesh_addr_t mfrom;

    mesh_data_t mdata = {
        .data  = (uint8_t *)buf,
        .size  = (uint16_t)*len,
        .proto = MESH_PROTO_BIN,
        .tos   = MESH_TOS_P2P,
    };

    int flag = 0;
    int block_ms = (timeout_ms < 0) ? (int)portMAX_DELAY : timeout_ms;

    esp_err_t esp_err = esp_mesh_recv(&mfrom, &mdata, block_ms, &flag, NULL, 0);
    if (esp_err == ESP_OK) {
        if (src != NULL) {
            memcpy(src->addr, mfrom.addr, 6);
        }
        *len = (size_t)mdata.size;
    }

    return blusys_translate_esp_err(esp_err);
}

bool blusys_wifi_mesh_is_root(blusys_wifi_mesh_t *handle)
{
    if (handle == NULL) {
        return false;
    }
    return esp_mesh_is_root();
}

blusys_err_t blusys_wifi_mesh_get_layer(blusys_wifi_mesh_t *handle, int *out_layer)
{
    if ((handle == NULL) || (out_layer == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    *out_layer = esp_mesh_get_layer();
    return BLUSYS_OK;
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_wifi_mesh_open(const blusys_wifi_mesh_config_t *config,
                                    blusys_wifi_mesh_t **out_handle)
{
    (void)config;
    (void)out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_mesh_close(blusys_wifi_mesh_t *handle)
{
    (void)handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_mesh_send(blusys_wifi_mesh_t *handle,
                                    const blusys_wifi_mesh_addr_t *dst,
                                    const void *data, size_t len)
{
    (void)handle;
    (void)dst;
    (void)data;
    (void)len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_mesh_recv(blusys_wifi_mesh_t *handle,
                                    blusys_wifi_mesh_addr_t *src,
                                    void *buf, size_t *len,
                                    int timeout_ms)
{
    (void)handle;
    (void)src;
    (void)buf;
    (void)len;
    (void)timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_wifi_mesh_is_root(blusys_wifi_mesh_t *handle)
{
    (void)handle;
    return false;
}

blusys_err_t blusys_wifi_mesh_get_layer(blusys_wifi_mesh_t *handle, int *out_layer)
{
    (void)handle;
    (void)out_layer;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_WIFI_SUPPORTED */
