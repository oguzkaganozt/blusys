#include "blusys/services/protocol/sntp.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "freertos/FreeRTOS.h"

#include "esp_netif_sntp.h"
#include "esp_sntp.h"

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/global_lock.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

struct blusys_sntp {
    blusys_sntp_sync_cb_t   sync_cb;
    void                   *user_ctx;
};

static blusys_sntp_t *s_active_handle;

BLUSYS_DEFINE_GLOBAL_LOCK(sntp);

static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    if (blusys_lock_take(&s_sntp_global_lock, 0) != BLUSYS_OK) {
        return;
    }
    blusys_sntp_t *h = s_active_handle;
    if (h && h->sync_cb) {
        h->sync_cb(h->user_ctx);
    }
    blusys_lock_give(&s_sntp_global_lock);
}

blusys_err_t blusys_sntp_open(const blusys_sntp_config_t *config, blusys_sntp_t **out_sntp)
{
    if (config == NULL || out_sntp == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t gerr = ensure_global_lock();
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    gerr = blusys_lock_take(&s_sntp_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    if (s_active_handle != NULL) {
        blusys_lock_give(&s_sntp_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    const char *server = config->server ? config->server : "pool.ntp.org";

    blusys_sntp_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_sntp_global_lock);
        return BLUSYS_ERR_NO_MEM;
    }

    h->sync_cb  = config->sync_cb;
    h->user_ctx = config->user_ctx;

    esp_sntp_config_t sntp_cfg;
    if (config->server2) {
        esp_sntp_config_t tmp = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
                                    ESP_SNTP_SERVER_LIST(server, config->server2));
        memcpy(&sntp_cfg, &tmp, sizeof(sntp_cfg));
    } else {
        esp_sntp_config_t tmp = ESP_NETIF_SNTP_DEFAULT_CONFIG(server);
        memcpy(&sntp_cfg, &tmp, sizeof(sntp_cfg));
    }

    sntp_cfg.smooth_sync = config->smooth_sync;
    sntp_cfg.sync_cb     = config->sync_cb ? sntp_sync_cb : NULL;

    s_active_handle = h;

    esp_err_t esp_err = esp_netif_sntp_init(&sntp_cfg);
    if (esp_err != ESP_OK) {
        s_active_handle = NULL;
        blusys_lock_give(&s_sntp_global_lock);
        free(h);
        return blusys_translate_esp_err(esp_err);
    }

    blusys_lock_give(&s_sntp_global_lock);
    *out_sntp = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_sntp_close(blusys_sntp_t *sntp)
{
    if (sntp == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_netif_sntp_deinit();

    blusys_err_t err = blusys_lock_take(&s_sntp_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    s_active_handle = NULL;
    blusys_lock_give(&s_sntp_global_lock);

    free(sntp);
    return BLUSYS_OK;
}

blusys_err_t blusys_sntp_sync(blusys_sntp_t *sntp, int timeout_ms)
{
    if (sntp == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    TickType_t ticks = blusys_timeout_ms_to_ticks(timeout_ms);

    esp_err_t esp_err = esp_netif_sntp_sync_wait(ticks);

    if (esp_err == ESP_OK) {
        return BLUSYS_OK;
    }
    if (esp_err == ESP_ERR_TIMEOUT) {
        return BLUSYS_ERR_TIMEOUT;
    }
    return blusys_translate_esp_err(esp_err);
}

bool blusys_sntp_is_synced(blusys_sntp_t *sntp)
{
    if (sntp == NULL) {
        return false;
    }
    return esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_sntp_open(const blusys_sntp_config_t *config, blusys_sntp_t **out_sntp)
{
    (void)config; (void)out_sntp;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sntp_close(blusys_sntp_t *sntp)
{
    (void)sntp;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sntp_sync(blusys_sntp_t *sntp, int timeout_ms)
{
    (void)sntp; (void)timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_sntp_is_synced(blusys_sntp_t *sntp)
{
    (void)sntp;
    return false;
}

#endif
