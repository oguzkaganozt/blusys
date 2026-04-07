#include "blusys/connectivity/wifi.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "lwip/ip4_addr.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_nvs_init.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

struct blusys_wifi {
    esp_netif_t                  *netif;
    esp_event_handler_instance_t  wifi_handler;
    esp_event_handler_instance_t  ip_handler;
    EventGroupHandle_t             event_group;
    blusys_lock_t                  lock;
    volatile bool                  connected;
    char                           ssid[33];
    char                           password[65];
};

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    blusys_wifi_t *h = arg;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        h->connected = false;
        xEventGroupSetBits(h->event_group, WIFI_FAIL_BIT);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        h->connected = true;
        xEventGroupSetBits(h->event_group, WIFI_CONNECTED_BIT);
    }
}

blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config, blusys_wifi_t **out_wifi)
{
    if ((config == NULL) || (config->ssid == NULL) || (out_wifi == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_wifi_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    h->event_group = xEventGroupCreate();
    if (h->event_group == NULL) {
        blusys_lock_deinit(&h->lock);
        free(h);
        return BLUSYS_ERR_NO_MEM;
    }

    strncpy(h->ssid, config->ssid, sizeof(h->ssid) - 1);
    if (config->password != NULL) {
        strncpy(h->password, config->password, sizeof(h->password) - 1);
    }

    /* NVS is required by the WiFi driver for RF calibration data */
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

    h->netif = esp_netif_create_default_wifi_sta();
    if (h->netif == NULL) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_netif_create;
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err = esp_wifi_init(&wifi_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_wifi_init;
    }

    esp_err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   wifi_event_handler, h, &h->wifi_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler_wifi;
    }

    esp_err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   wifi_event_handler, h, &h->ip_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler_ip;
    }

    wifi_config_t sta_cfg = {};
    strncpy((char *)sta_cfg.sta.ssid, h->ssid, sizeof(sta_cfg.sta.ssid) - 1);
    strncpy((char *)sta_cfg.sta.password, h->password, sizeof(sta_cfg.sta.password) - 1);

    esp_err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_set_mode;
    }

    esp_err = esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_set_mode;
    }

    esp_err = esp_wifi_start();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_set_mode;
    }

    *out_wifi = h;
    return BLUSYS_OK;

fail_set_mode:
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, h->ip_handler);
fail_handler_ip:
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, h->wifi_handler);
fail_handler_wifi:
    esp_wifi_deinit();
fail_wifi_init:
    esp_netif_destroy(h->netif);
fail_netif_create:
    esp_event_loop_delete_default();
fail_event_loop:
    esp_netif_deinit();
fail_netif_init:
fail_nvs:
    vEventGroupDelete(h->event_group);
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_wifi_close(blusys_wifi_t *wifi)
{
    if (wifi == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (wifi->connected) {
        esp_wifi_disconnect();
    }

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi->wifi_handler);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi->ip_handler);

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy(wifi->netif);
    esp_event_loop_delete_default();
    esp_netif_deinit();

    vEventGroupDelete(wifi->event_group);

    blusys_lock_give(&wifi->lock);
    blusys_lock_deinit(&wifi->lock);
    free(wifi);
    return BLUSYS_OK;
}

blusys_err_t blusys_wifi_connect(blusys_wifi_t *wifi, int timeout_ms)
{
    if (wifi == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    xEventGroupClearBits(wifi->event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    esp_err_t esp_err = esp_wifi_connect();

    blusys_lock_give(&wifi->lock);

    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    TickType_t ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS((uint32_t)timeout_ms);
    EventBits_t bits = xEventGroupWaitBits(wifi->event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE, pdFALSE, ticks);

    if (bits & WIFI_CONNECTED_BIT) {
        return BLUSYS_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        return BLUSYS_ERR_IO;
    } else {
        return BLUSYS_ERR_TIMEOUT;
    }
}

blusys_err_t blusys_wifi_disconnect(blusys_wifi_t *wifi)
{
    if (wifi == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = esp_wifi_disconnect();
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&wifi->lock);
    return err;
}

blusys_err_t blusys_wifi_get_ip_info(blusys_wifi_t *wifi, blusys_wifi_ip_info_t *out_info)
{
    if ((wifi == NULL) || (out_info == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!wifi->connected) {
        blusys_lock_give(&wifi->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t esp_err = esp_netif_get_ip_info(wifi->netif, &ip_info);
    if (esp_err != ESP_OK) {
        blusys_lock_give(&wifi->lock);
        return blusys_translate_esp_err(esp_err);
    }

    snprintf(out_info->ip,      sizeof(out_info->ip),      IPSTR, IP2STR(&ip_info.ip));
    snprintf(out_info->netmask, sizeof(out_info->netmask),  IPSTR, IP2STR(&ip_info.netmask));
    snprintf(out_info->gateway, sizeof(out_info->gateway),  IPSTR, IP2STR(&ip_info.gw));

    blusys_lock_give(&wifi->lock);
    return BLUSYS_OK;
}

bool blusys_wifi_is_connected(blusys_wifi_t *wifi)
{
    if (wifi == NULL) {
        return false;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return false;
    }

    bool connected = wifi->connected;

    blusys_lock_give(&wifi->lock);
    return connected;
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config, blusys_wifi_t **out_wifi)
{
    (void) config;
    (void) out_wifi;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_close(blusys_wifi_t *wifi)
{
    (void) wifi;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_connect(blusys_wifi_t *wifi, int timeout_ms)
{
    (void) wifi;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_disconnect(blusys_wifi_t *wifi)
{
    (void) wifi;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_wifi_get_ip_info(blusys_wifi_t *wifi, blusys_wifi_ip_info_t *out_info)
{
    (void) wifi;
    (void) out_info;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_wifi_is_connected(blusys_wifi_t *wifi)
{
    (void) wifi;
    return false;
}

#endif /* SOC_WIFI_SUPPORTED */
