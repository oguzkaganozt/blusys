#include "blusys/connectivity/wifi_prov.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "wifi_provisioning/scheme_ble.h"
#endif

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_nvs_init.h"

struct blusys_wifi_prov {
    blusys_wifi_prov_transport_t  transport;
    blusys_wifi_prov_cb_t         on_event;
    void                         *user_ctx;
    blusys_lock_t                 lock;
    volatile bool                 running;
    char                          service_name[33];
    char                          pop[65];
    char                          service_key[65];
    esp_event_handler_instance_t  prov_handler;
    esp_netif_t                  *sta_netif;
    esp_netif_t                  *ap_netif;   /* non-NULL for SoftAP transport only */
};

static blusys_wifi_prov_t *s_prov_handle = NULL;

static void prov_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    (void)arg;
    (void)base;

    blusys_wifi_prov_t *h = s_prov_handle;
    if (h == NULL || h->on_event == NULL) {
        return;
    }

    switch ((wifi_prov_cb_event_t)id) {
        case WIFI_PROV_START: {
            h->on_event(BLUSYS_WIFI_PROV_EVENT_STARTED, NULL, h->user_ctx);
            break;
        }
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *sta = (wifi_sta_config_t *)data;
            blusys_wifi_prov_credentials_t creds;
            memset(&creds, 0, sizeof(creds));
            strncpy(creds.ssid,     (char *)sta->ssid,     sizeof(creds.ssid) - 1);
            strncpy(creds.password, (char *)sta->password, sizeof(creds.password) - 1);
            h->on_event(BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED, &creds, h->user_ctx);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS: {
            h->on_event(BLUSYS_WIFI_PROV_EVENT_SUCCESS, NULL, h->user_ctx);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            h->on_event(BLUSYS_WIFI_PROV_EVENT_FAILED, NULL, h->user_ctx);
            break;
        }
        default:
            break;
    }
}

blusys_err_t blusys_wifi_prov_open(const blusys_wifi_prov_config_t *config,
                                   blusys_wifi_prov_t **out_prov)
{
    if (config == NULL || config->service_name == NULL ||
        config->on_event == NULL || out_prov == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

#if !CONFIG_BT_NIMBLE_ENABLED
    if (config->transport == BLUSYS_WIFI_PROV_TRANSPORT_BLE) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }
#endif

    if (s_prov_handle != NULL) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_wifi_prov_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    h->transport = config->transport;
    h->on_event  = config->on_event;
    h->user_ctx  = config->user_ctx;
    strncpy(h->service_name, config->service_name, sizeof(h->service_name) - 1);
    if (config->pop != NULL) {
        strncpy(h->pop, config->pop, sizeof(h->pop) - 1);
    }
    if (config->service_key != NULL) {
        strncpy(h->service_key, config->service_key, sizeof(h->service_key) - 1);
    }

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

    h->sta_netif = esp_netif_create_default_wifi_sta();
    if (h->sta_netif == NULL) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_netif_create;
    }

    if (config->transport == BLUSYS_WIFI_PROV_TRANSPORT_SOFTAP) {
        h->ap_netif = esp_netif_create_default_wifi_ap();
        if (h->ap_netif == NULL) {
            err = BLUSYS_ERR_INTERNAL;
            goto fail_ap_netif;
        }
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

    wifi_prov_mgr_config_t mgr_cfg = {};

#if CONFIG_BT_NIMBLE_ENABLED
    if (h->transport == BLUSYS_WIFI_PROV_TRANSPORT_BLE) {
        mgr_cfg.scheme               = wifi_prov_scheme_ble;
        mgr_cfg.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;
    } else {
#endif
        mgr_cfg.scheme               = wifi_prov_scheme_softap;
        mgr_cfg.scheme_event_handler = (wifi_prov_event_handler_t)WIFI_PROV_EVENT_HANDLER_NONE;
#if CONFIG_BT_NIMBLE_ENABLED
    }
#endif

    esp_err = wifi_prov_mgr_init(mgr_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_mode;
    }

    esp_err = esp_event_handler_instance_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                                   prov_event_handler, NULL,
                                                   &h->prov_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler;
    }

    s_prov_handle = h;
    *out_prov = h;
    return BLUSYS_OK;

fail_handler:
    wifi_prov_mgr_deinit();
fail_wifi_mode:
    esp_wifi_stop();
    esp_wifi_deinit();
fail_wifi_init:
    if (h->ap_netif != NULL) {
        esp_netif_destroy(h->ap_netif);
    }
fail_ap_netif:
    esp_netif_destroy(h->sta_netif);
fail_netif_create:
    esp_event_loop_delete_default();
fail_event_loop:
    esp_netif_deinit();
fail_netif_init:
fail_nvs:
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_wifi_prov_close(blusys_wifi_prov_t *prov)
{
    if (prov == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&prov->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (prov->running) {
        wifi_prov_mgr_stop_provisioning();
        prov->running = false;
    }

    blusys_lock_give(&prov->lock);

    esp_event_handler_instance_unregister(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                          prov->prov_handler);
    wifi_prov_mgr_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    if (prov->ap_netif != NULL) {
        esp_netif_destroy(prov->ap_netif);
    }
    esp_netif_destroy(prov->sta_netif);
    esp_event_loop_delete_default();
    esp_netif_deinit();

    s_prov_handle = NULL;
    blusys_lock_deinit(&prov->lock);
    free(prov);
    return BLUSYS_OK;
}

blusys_err_t blusys_wifi_prov_start(blusys_wifi_prov_t *prov)
{
    if (prov == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&prov->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (prov->running) {
        blusys_lock_give(&prov->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    wifi_prov_security_t security = (prov->pop[0] != '\0')
                                  ? WIFI_PROV_SECURITY_1
                                  : WIFI_PROV_SECURITY_0;

    const char *pop         = (prov->pop[0] != '\0') ? prov->pop : NULL;
    const char *service_key = (prov->service_key[0] != '\0') ? prov->service_key : NULL;

    prov->running = true;
    blusys_lock_give(&prov->lock);

    esp_err_t esp_err = wifi_prov_mgr_start_provisioning(security, pop,
                                                          prov->service_name,
                                                          service_key);
    if (esp_err != ESP_OK) {
        if (blusys_lock_take(&prov->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
            prov->running = false;
            blusys_lock_give(&prov->lock);
        }
        return blusys_translate_esp_err(esp_err);
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_wifi_prov_stop(blusys_wifi_prov_t *prov)
{
    if (prov == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&prov->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (prov->running) {
        wifi_prov_mgr_stop_provisioning();
        prov->running = false;
    }

    blusys_lock_give(&prov->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_wifi_prov_get_qr_payload(blusys_wifi_prov_t *prov,
                                              char *buf, size_t buf_size)
{
    if (prov == NULL || buf == NULL || buf_size == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    const char *transport_str = (prov->transport == BLUSYS_WIFI_PROV_TRANSPORT_BLE)
                               ? "ble" : "softap";

    snprintf(buf, buf_size,
             "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
             prov->service_name,
             prov->pop,
             transport_str);

    return BLUSYS_OK;
}

bool blusys_wifi_prov_is_provisioned(void)
{
    wifi_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (esp_wifi_get_config(WIFI_IF_STA, &cfg) != ESP_OK) {
        return false;
    }
    return (strlen((char *)cfg.sta.ssid) > 0);
}

blusys_err_t blusys_wifi_prov_reset(void)
{
    esp_err_t esp_err = esp_wifi_restore();
    return blusys_translate_esp_err(esp_err);
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_wifi_prov_open(const blusys_wifi_prov_config_t *config,
                                   blusys_wifi_prov_t **out_prov)
{
    (void)config; (void)out_prov;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_prov_close(blusys_wifi_prov_t *prov)
{
    (void)prov;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_prov_start(blusys_wifi_prov_t *prov)
{
    (void)prov;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_prov_stop(blusys_wifi_prov_t *prov)
{
    (void)prov;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_prov_get_qr_payload(blusys_wifi_prov_t *prov,
                                              char *buf, size_t buf_size)
{
    (void)prov; (void)buf; (void)buf_size;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_wifi_prov_is_provisioned(void)
{
    return false;
}

blusys_err_t blusys_wifi_prov_reset(void)
{
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_WIFI_SUPPORTED */
