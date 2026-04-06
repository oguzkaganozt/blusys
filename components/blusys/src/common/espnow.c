#include "blusys/espnow.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_nvs_init.h"

#define ESPNOW_MAX_DATA_LEN ESP_NOW_MAX_DATA_LEN  /* 250 bytes */

struct blusys_espnow {
    blusys_lock_t            lock;
    blusys_espnow_recv_cb_t  recv_cb;
    void                    *recv_user_ctx;
    blusys_espnow_send_cb_t  send_cb;
    void                    *send_user_ctx;
};

/* ESP-NOW callbacks have no user-data pointer; use a static handle ref. */
static blusys_lock_t    s_espnow_global_lock;
static bool             s_espnow_global_lock_inited;
static blusys_espnow_t *s_handle;

static blusys_err_t ensure_global_lock(void)
{
    if (!s_espnow_global_lock_inited) {
        blusys_err_t err = blusys_lock_init(&s_espnow_global_lock);
        if (err != BLUSYS_OK) {
            return err;
        }
        s_espnow_global_lock_inited = true;
    }
    return BLUSYS_OK;
}

static void espnow_recv_cb(const esp_now_recv_info_t *info,
                           const uint8_t *data, int len)
{
    if (blusys_lock_take(&s_espnow_global_lock, 0) != BLUSYS_OK) {
        return;
    }
    blusys_espnow_t *h = s_handle;
    if (h && h->recv_cb) {
        h->recv_cb(info->src_addr, data, (size_t)len, h->recv_user_ctx);
    }
    blusys_lock_give(&s_espnow_global_lock);
}

static void espnow_send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    if (blusys_lock_take(&s_espnow_global_lock, 0) != BLUSYS_OK) {
        return;
    }
    blusys_espnow_t *h = s_handle;
    if (h && h->send_cb) {
        h->send_cb(info->des_addr, status == ESP_NOW_SEND_SUCCESS, h->send_user_ctx);
    }
    blusys_lock_give(&s_espnow_global_lock);
}

blusys_err_t blusys_espnow_open(const blusys_espnow_config_t *config,
                                blusys_espnow_t **out_handle)
{
    if ((config == NULL) || (config->recv_cb == NULL) || (out_handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t gerr = ensure_global_lock();
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    gerr = blusys_lock_take(&s_espnow_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    if (s_handle != NULL) {
        blusys_lock_give(&s_espnow_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_espnow_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_espnow_global_lock);
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_espnow_global_lock);
        free(h);
        return err;
    }

    h->recv_cb       = config->recv_cb;
    h->recv_user_ctx = config->recv_user_ctx;
    h->send_cb       = config->send_cb;
    h->send_user_ctx = config->send_user_ctx;

    /* NVS required by the WiFi driver for RF calibration data */
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

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err = esp_wifi_init(&wifi_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_init;
    }

    esp_err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_mode;
    }

    esp_err = esp_wifi_start();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_mode;
    }

    if (config->channel != 0) {
        esp_err = esp_wifi_set_channel(config->channel, WIFI_SECOND_CHAN_NONE);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_wifi_start;
        }
    }

    esp_err = esp_now_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_start;
    }

    esp_err = esp_now_register_recv_cb(espnow_recv_cb);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_espnow_init;
    }

    esp_err = esp_now_register_send_cb(espnow_send_cb);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_espnow_init;
    }

    s_handle  = h;
    blusys_lock_give(&s_espnow_global_lock);

    *out_handle = h;
    return BLUSYS_OK;

fail_espnow_init:
    esp_now_deinit();
fail_wifi_start:
    esp_wifi_stop();
fail_wifi_mode:
    esp_wifi_deinit();
fail_wifi_init:
    esp_event_loop_delete_default();
fail_event_loop:
    esp_netif_deinit();
fail_netif_init:
fail_nvs:
    blusys_lock_give(&s_espnow_global_lock);
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_espnow_close(blusys_espnow_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();

    blusys_lock_take(&s_espnow_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    s_handle = NULL;
    blusys_lock_give(&s_espnow_global_lock);

    blusys_lock_give(&handle->lock);
    blusys_lock_deinit(&handle->lock);
    free(handle);
    return BLUSYS_OK;
}

blusys_err_t blusys_espnow_add_peer(blusys_espnow_t *handle,
                                    const blusys_espnow_peer_t *peer)
{
    if ((handle == NULL) || (peer == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_now_peer_info_t info = {};
    memcpy(info.peer_addr, peer->addr, 6);
    info.channel = peer->channel;
    info.encrypt = peer->encrypt;
    if (peer->encrypt) {
        memcpy(info.lmk, peer->lmk, sizeof(info.lmk));
    }
    info.ifidx = WIFI_IF_STA;

    esp_err_t esp_err = esp_now_add_peer(&info);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_espnow_remove_peer(blusys_espnow_t *handle,
                                       const uint8_t addr[6])
{
    if ((handle == NULL) || (addr == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err_t esp_err = esp_now_del_peer(addr);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_espnow_send(blusys_espnow_t *handle,
                                const uint8_t addr[6],
                                const void *data, size_t size)
{
    if ((handle == NULL) || (addr == NULL) || (data == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (size > ESPNOW_MAX_DATA_LEN) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err_t esp_err = esp_now_send(addr, (const uint8_t *)data, size);
    return blusys_translate_esp_err(esp_err);
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_espnow_open(const blusys_espnow_config_t *config,
                                blusys_espnow_t **out_handle)
{
    (void) config;
    (void) out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_espnow_close(blusys_espnow_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_espnow_add_peer(blusys_espnow_t *handle,
                                    const blusys_espnow_peer_t *peer)
{
    (void) handle;
    (void) peer;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_espnow_remove_peer(blusys_espnow_t *handle,
                                       const uint8_t addr[6])
{
    (void) handle;
    (void) addr;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_espnow_send(blusys_espnow_t *handle,
                                const uint8_t addr[6],
                                const void *data, size_t size)
{
    (void) handle;
    (void) addr;
    (void) data;
    (void) size;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_WIFI_SUPPORTED */
