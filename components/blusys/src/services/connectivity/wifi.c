#include "blusys/services/connectivity/wifi.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if !(SOC_WIFI_SUPPORTED)
#error "blusys_wifi requires SOC_WIFI_SUPPORTED"
#endif


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "lwip/ip4_addr.h"

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/services/internal/net_bootstrap.h"

#define WIFI_CONNECTED_BIT            BIT0
#define WIFI_FAIL_BIT                 BIT1
#define WIFI_WORKER_EXITED_BIT        BIT2
#define WIFI_CONNECT_DONE_BIT         BIT3
#define BLUSYS_WIFI_DEFAULT_RETRY_MS  1000

struct blusys_wifi {
    esp_netif_t                      *netif;
    blusys_net_bootstrap_t            bootstrap;
    esp_event_handler_instance_t      wifi_handler;
    esp_event_handler_instance_t      ip_handler;
    EventGroupHandle_t                event_group;
    TaskHandle_t                      reconnect_task;
    blusys_lock_t                     lock;
    blusys_wifi_event_cb_t            on_event;
    void                             *user_ctx;
    volatile bool                     connected;
    volatile bool                     closing;
    volatile bool                     manual_disconnect;
    volatile bool                     ever_connected;
    volatile bool                     connect_in_progress;
    volatile bool                     connect_waiting;
    bool                              auto_reconnect;
    int                               reconnect_delay_ms;
    int                               max_retries;
    volatile int                      retry_count;
    volatile blusys_wifi_disconnect_reason_t last_disconnect_reason;
    volatile int                      last_disconnect_reason_raw;
    char                              ssid[33];
    char                              password[65];
};

static blusys_wifi_disconnect_reason_t map_disconnect_reason(uint8_t reason)
{
    switch (reason) {
        case WIFI_REASON_AUTH_EXPIRE:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_AUTH_FAIL:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_AUTH_LEAVE:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_802_1X_AUTH_FAILED:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_ASSOC_NOT_AUTHED:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_IE_INVALID:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_MIC_FAILURE:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_IE_IN_4WAY_DIFFERS:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_GROUP_CIPHER_INVALID:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_AKMP_INVALID:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_INVALID_RSN_IE_CAP:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_CIPHER_SUITE_REJECTED:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_BAD_CIPHER_OR_AKM:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_INVALID_PMKID:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_INVALID_MDE:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;
        case WIFI_REASON_INVALID_FTE:
            return BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED;

        case WIFI_REASON_NO_AP_FOUND:
            return BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND;
        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
            return BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND;
        case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
            return BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND;
        case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
            return BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND;

        case WIFI_REASON_ASSOC_FAIL:
            return BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED;
        case WIFI_REASON_ASSOC_EXPIRE:
            return BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED;
        case WIFI_REASON_ASSOC_TOOMANY:
            return BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED;
        case WIFI_REASON_DISASSOC_PWRCAP_BAD:
            return BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED;
        case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
            return BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED;
        case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:
            return BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED;

        case WIFI_REASON_BEACON_TIMEOUT:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_CONNECTION_FAIL:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_NOT_AUTHED:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_NOT_ASSOCED:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_ASSOC_LEAVE:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_TIMEOUT:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_PEER_INITIATED:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_AP_INITIATED:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_AP_TSF_RESET:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_ROAMING:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_SA_QUERY_TIMEOUT:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;
        case WIFI_REASON_STA_LEAVING:
            return BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST;

        default:
            return BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN;
    }
}

static bool retry_allowed(const blusys_wifi_t *wifi)
{
    if (!wifi->auto_reconnect) {
        return false;
    }
    if (wifi->connect_in_progress) {
        return false;
    }
    if (wifi->closing || wifi->manual_disconnect || !wifi->ever_connected) {
        return false;
    }
    if (wifi->max_retries == 0) {
        return false;
    }
    if ((wifi->max_retries > 0) && (wifi->retry_count >= wifi->max_retries)) {
        return false;
    }
    return true;
}

static blusys_err_t fill_ip_info(blusys_wifi_t *wifi, blusys_wifi_ip_info_t *out_info)
{
    memset(out_info, 0, sizeof(*out_info));

    esp_netif_ip_info_t ip_info;
    esp_err_t esp_err = esp_netif_get_ip_info(wifi->netif, &ip_info);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    snprintf(out_info->ip, sizeof(out_info->ip), IPSTR, IP2STR(&ip_info.ip));
    snprintf(out_info->netmask, sizeof(out_info->netmask), IPSTR, IP2STR(&ip_info.netmask));
    snprintf(out_info->gateway, sizeof(out_info->gateway), IPSTR, IP2STR(&ip_info.gw));
    return BLUSYS_OK;
}

static void emit_event(blusys_wifi_t *wifi, blusys_wifi_event_t event,
                       const blusys_wifi_event_info_t *info)
{
    if (wifi->on_event != NULL) {
        wifi->on_event(wifi, event, info, wifi->user_ctx);
    }
}

static void emit_simple_event(blusys_wifi_t *wifi, blusys_wifi_event_t event)
{
    emit_event(wifi, event, NULL);
}

static void finish_connect_wait(blusys_wifi_t *wifi)
{
    wifi->connect_waiting = false;
    xEventGroupSetBits(wifi->event_group, WIFI_CONNECT_DONE_BIT);
}

static void reconnect_task_main(void *arg)
{
    blusys_wifi_t *wifi = (blusys_wifi_t *)arg;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (wifi->closing) {
            break;
        }
        if (!retry_allowed(wifi)) {
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS((uint32_t)wifi->reconnect_delay_ms));

        blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            continue;
        }

        if (wifi->closing || !retry_allowed(wifi)) {
            blusys_lock_give(&wifi->lock);
            continue;
        }

        wifi->retry_count++;
        wifi->connect_in_progress = true;
        xEventGroupClearBits(wifi->event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

        int retry_attempt = wifi->retry_count;
        blusys_wifi_disconnect_reason_t disconnect_reason = wifi->last_disconnect_reason;
        int raw_disconnect_reason = wifi->last_disconnect_reason_raw;

        blusys_lock_give(&wifi->lock);

        blusys_wifi_event_info_t info;
        memset(&info, 0, sizeof(info));
        info.retry_attempt = retry_attempt;
        info.disconnect_reason = disconnect_reason;
        info.raw_disconnect_reason = raw_disconnect_reason;
        emit_event(wifi, BLUSYS_WIFI_EVENT_RECONNECTING, &info);

        err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            continue;
        }
        bool reconnect_cancelled = wifi->closing || wifi->manual_disconnect || !wifi->connect_in_progress;
        blusys_lock_give(&wifi->lock);
        if (reconnect_cancelled) {
            continue;
        }

        emit_simple_event(wifi, BLUSYS_WIFI_EVENT_CONNECTING);

        err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err != BLUSYS_OK) {
            continue;
        }
        reconnect_cancelled = wifi->closing || wifi->manual_disconnect || !wifi->connect_in_progress;
        blusys_lock_give(&wifi->lock);
        if (reconnect_cancelled) {
            continue;
        }

        esp_err_t esp_err = esp_wifi_connect();
        if (esp_err != ESP_OK) {
            err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
            if (err == BLUSYS_OK) {
                wifi->connect_in_progress = false;
                blusys_lock_give(&wifi->lock);
            }
        }
    }

    xEventGroupSetBits(wifi->event_group, WIFI_WORKER_EXITED_BIT);
    vTaskDelete(NULL);
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    blusys_wifi_t *wifi = arg;

    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_STA_START) {
            emit_simple_event(wifi, BLUSYS_WIFI_EVENT_STARTED);
            return;
        }

        if (id == WIFI_EVENT_STA_CONNECTED) {
            emit_simple_event(wifi, BLUSYS_WIFI_EVENT_CONNECTED);
            return;
        }

        if (id == WIFI_EVENT_STA_DISCONNECTED) {
            const wifi_event_sta_disconnected_t *disc = (const wifi_event_sta_disconnected_t *)data;
            bool was_manual = false;
            bool closing = false;
            bool should_retry = false;
            blusys_wifi_disconnect_reason_t disconnect_reason = BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN;
            int raw_disconnect_reason = 0;

            blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
            if (err == BLUSYS_OK) {
                was_manual = wifi->manual_disconnect;
                wifi->connected = false;
                wifi->connect_in_progress = false;

                if (was_manual) {
                    disconnect_reason = BLUSYS_WIFI_DISCONNECT_REASON_USER_REQUESTED;
                    wifi->manual_disconnect = false;
                } else if (disc != NULL) {
                    raw_disconnect_reason = (int)disc->reason;
                    disconnect_reason = map_disconnect_reason(disc->reason);
                }

                wifi->last_disconnect_reason = disconnect_reason;
                wifi->last_disconnect_reason_raw = raw_disconnect_reason;
                closing = wifi->closing;
                should_retry = !was_manual && retry_allowed(wifi);
                blusys_lock_give(&wifi->lock);
            } else if (disc != NULL) {
                raw_disconnect_reason = (int)disc->reason;
                disconnect_reason = map_disconnect_reason(disc->reason);
            }

            xEventGroupSetBits(wifi->event_group, WIFI_FAIL_BIT);

            if (closing) {
                return;
            }

            blusys_wifi_event_info_t info;
            memset(&info, 0, sizeof(info));
            info.disconnect_reason = disconnect_reason;
            info.raw_disconnect_reason = raw_disconnect_reason;
            emit_event(wifi, BLUSYS_WIFI_EVENT_DISCONNECTED, &info);

            if (should_retry) {
                xTaskNotifyGive(wifi->reconnect_task);
            }
            return;
        }
    }

    if ((base == IP_EVENT) && (id == IP_EVENT_STA_GOT_IP)) {
        blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
        if (err == BLUSYS_OK) {
            wifi->connected = true;
            wifi->ever_connected = true;
            wifi->connect_in_progress = false;
            wifi->retry_count = 0;
            blusys_lock_give(&wifi->lock);
        }
        xEventGroupSetBits(wifi->event_group, WIFI_CONNECTED_BIT);

        blusys_wifi_event_info_t info;
        memset(&info, 0, sizeof(info));
        (void)fill_ip_info(wifi, &info.ip_info);
        emit_event(wifi, BLUSYS_WIFI_EVENT_GOT_IP, &info);
    }
}

blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config, blusys_wifi_t **out_wifi)
{
    if ((config == NULL) || (config->ssid == NULL) || (out_wifi == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_wifi_t *wifi = calloc(1, sizeof(*wifi));
    if (wifi == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&wifi->lock);
    if (err != BLUSYS_OK) {
        free(wifi);
        return err;
    }

    wifi->event_group = xEventGroupCreate();
    if (wifi->event_group == NULL) {
        blusys_lock_deinit(&wifi->lock);
        free(wifi);
        return BLUSYS_ERR_NO_MEM;
    }

    BaseType_t task_ok = xTaskCreate(reconnect_task_main, "blusys_wifi_reconnect",
                                     3072, wifi, tskIDLE_PRIORITY + 1, &wifi->reconnect_task);
    if (task_ok != pdPASS) {
        vEventGroupDelete(wifi->event_group);
        blusys_lock_deinit(&wifi->lock);
        free(wifi);
        return BLUSYS_ERR_NO_MEM;
    }

    wifi->on_event = config->on_event;
    wifi->user_ctx = config->user_ctx;
    wifi->auto_reconnect = config->auto_reconnect;
    wifi->reconnect_delay_ms = (config->reconnect_delay_ms > 0)
                             ? config->reconnect_delay_ms
                             : BLUSYS_WIFI_DEFAULT_RETRY_MS;
    wifi->max_retries = config->max_retries;
    wifi->last_disconnect_reason = BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN;
    wifi->last_disconnect_reason_raw = 0;

    strncpy(wifi->ssid, config->ssid, sizeof(wifi->ssid) - 1);
    if (config->password != NULL) {
        strncpy(wifi->password, config->password, sizeof(wifi->password) - 1);
    }

    err = blusys_net_bootstrap_start_wifi(&wifi->bootstrap);
    if (err != BLUSYS_OK) {
        goto fail_bootstrap;
    }

    wifi->netif = blusys_net_bootstrap_create_wifi_sta();
    if (wifi->netif == NULL) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_netif_create;
    }

    esp_err_t esp_err;

    esp_err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                  wifi_event_handler, wifi, &wifi->wifi_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler_wifi;
    }

    esp_err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                  wifi_event_handler, wifi, &wifi->ip_handler);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_handler_ip;
    }

    wifi_config_t sta_cfg = { 0 };
    strncpy((char *)sta_cfg.sta.ssid, wifi->ssid, sizeof(sta_cfg.sta.ssid) - 1);
    strncpy((char *)sta_cfg.sta.password, wifi->password, sizeof(sta_cfg.sta.password) - 1);
    sta_cfg.sta.pmf_cfg.capable = true;
    sta_cfg.sta.pmf_cfg.required = false;
    sta_cfg.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

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

    *out_wifi = wifi;
    return BLUSYS_OK;

fail_set_mode:
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi->ip_handler);
fail_handler_ip:
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi->wifi_handler);
fail_handler_wifi:
    esp_netif_destroy(wifi->netif);
fail_netif_create:
    blusys_net_bootstrap_stop(&wifi->bootstrap);
fail_bootstrap:
    vTaskDelete(wifi->reconnect_task);
    vEventGroupDelete(wifi->event_group);
    blusys_lock_deinit(&wifi->lock);
    free(wifi);
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

    wifi->closing = true;
    wifi->manual_disconnect = false;
    wifi->connect_in_progress = false;
    bool wait_for_connect = wifi->connect_waiting;

    blusys_lock_give(&wifi->lock);

    if (wait_for_connect) {
        xEventGroupSetBits(wifi->event_group, WIFI_FAIL_BIT);
        xEventGroupWaitBits(wifi->event_group, WIFI_CONNECT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    }

    xTaskAbortDelay(wifi->reconnect_task);
    xTaskNotifyGive(wifi->reconnect_task);
    xEventGroupWaitBits(wifi->event_group, WIFI_WORKER_EXITED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    if (wifi->connected) {
        esp_wifi_disconnect();
    }

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi->wifi_handler);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi->ip_handler);

    esp_wifi_stop();

    wifi->connected = false;
    wifi->connect_in_progress = false;
    emit_simple_event(wifi, BLUSYS_WIFI_EVENT_STOPPED);

    esp_netif_destroy(wifi->netif);
    blusys_net_bootstrap_stop(&wifi->bootstrap);

    vEventGroupDelete(wifi->event_group);

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

    if (wifi->closing) {
        blusys_lock_give(&wifi->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    if (wifi->connect_in_progress) {
        blusys_lock_give(&wifi->lock);
        return BLUSYS_ERR_BUSY;
    }

    wifi->manual_disconnect = false;
    wifi->connect_in_progress = true;
    wifi->connect_waiting = true;
    xEventGroupClearBits(wifi->event_group, WIFI_CONNECT_DONE_BIT);
    xEventGroupClearBits(wifi->event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    blusys_lock_give(&wifi->lock);

    xTaskAbortDelay(wifi->reconnect_task);
    emit_simple_event(wifi, BLUSYS_WIFI_EVENT_CONNECTING);

    if (wifi->closing || wifi->manual_disconnect || !wifi->connect_in_progress) {
        finish_connect_wait(wifi);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_err_t esp_err = esp_wifi_connect();
    if (esp_err != ESP_OK) {
        wifi->connect_in_progress = false;
        finish_connect_wait(wifi);
        return blusys_translate_esp_err(esp_err);
    }

    TickType_t ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS((uint32_t)timeout_ms);
    EventBits_t bits = xEventGroupWaitBits(wifi->event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, ticks);

    if (bits & WIFI_CONNECTED_BIT) {
        finish_connect_wait(wifi);
        return BLUSYS_OK;
    }
    if (bits & WIFI_FAIL_BIT) {
        finish_connect_wait(wifi);
        return BLUSYS_ERR_IO;
    }
    wifi->connect_in_progress = false;
    finish_connect_wait(wifi);
    return BLUSYS_ERR_TIMEOUT;
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

    if (wifi->closing) {
        blusys_lock_give(&wifi->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    wifi->manual_disconnect = true;
    wifi->connect_in_progress = false;
    wifi->last_disconnect_reason = BLUSYS_WIFI_DISCONNECT_REASON_USER_REQUESTED;
    wifi->last_disconnect_reason_raw = 0;

    blusys_lock_give(&wifi->lock);

    xTaskAbortDelay(wifi->reconnect_task);

    esp_err_t esp_err = esp_wifi_disconnect();
    return blusys_translate_esp_err(esp_err);
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

    err = fill_ip_info(wifi, out_info);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&wifi->lock);
        return err;
    }

    blusys_lock_give(&wifi->lock);
    return BLUSYS_OK;
}

blusys_wifi_disconnect_reason_t blusys_wifi_get_last_disconnect_reason(blusys_wifi_t *wifi)
{
    if (wifi == NULL) {
        return BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN;
    }

    blusys_wifi_disconnect_reason_t reason = wifi->last_disconnect_reason;

    blusys_lock_give(&wifi->lock);
    return reason;
}

int blusys_wifi_get_last_disconnect_reason_raw(blusys_wifi_t *wifi)
{
    if (wifi == NULL) {
        return 0;
    }

    blusys_err_t err = blusys_lock_take(&wifi->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return 0;
    }

    int reason = wifi->last_disconnect_reason_raw;

    blusys_lock_give(&wifi->lock);
    return reason;
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

