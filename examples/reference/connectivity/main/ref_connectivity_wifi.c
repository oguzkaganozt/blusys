#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "blusys/blusys.h"

#if CONFIG_CONNECTIVITY_SCENARIO_WIFI_CONNECT

static const char *wifi_event_name(blusys_wifi_event_t event)
{
    switch (event) {
        case BLUSYS_WIFI_EVENT_STARTED:
            return "started";
        case BLUSYS_WIFI_EVENT_CONNECTING:
            return "connecting";
        case BLUSYS_WIFI_EVENT_CONNECTED:
            return "connected";
        case BLUSYS_WIFI_EVENT_GOT_IP:
            return "got_ip";
        case BLUSYS_WIFI_EVENT_DISCONNECTED:
            return "disconnected";
        case BLUSYS_WIFI_EVENT_RECONNECTING:
            return "reconnecting";
        case BLUSYS_WIFI_EVENT_STOPPED:
            return "stopped";
        default:
            return "unknown";
    }
}

static const char *disconnect_reason_name(blusys_wifi_disconnect_reason_t reason)
{
    switch (reason) {
        case BLUSYS_WIFI_DISCONNECT_REASON_USER_REQUESTED:
            return "user_requested";
        case BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED:
            return "auth_failed";
        case BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND:
            return "no_ap_found";
        case BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED:
            return "assoc_failed";
        case BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST:
            return "connection_lost";
        case BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN:
        default:
            return "unknown";
    }
}

static void on_wifi_event(blusys_wifi_t *wifi, blusys_wifi_event_t event,
                          const blusys_wifi_event_info_t *info, void *user_ctx)
{
    (void) wifi;
    (void) user_ctx;

    printf("[wifi] event: %s\n", wifi_event_name(event));

    if ((event == BLUSYS_WIFI_EVENT_GOT_IP) && (info != NULL)) {
        printf("[wifi] ip=%s gw=%s\n", info->ip_info.ip, info->ip_info.gateway);
    }

    if ((event == BLUSYS_WIFI_EVENT_DISCONNECTED) && (info != NULL)) {
        printf("[wifi] disconnect reason: %s",
               disconnect_reason_name(info->disconnect_reason));
        if (info->raw_disconnect_reason != 0) {
            printf(" (%d)", info->raw_disconnect_reason);
        }
        printf("\n");
    }

    if ((event == BLUSYS_WIFI_EVENT_RECONNECTING) && (info != NULL)) {
        printf("[wifi] reconnect attempt #%d after %s",
               info->retry_attempt,
               disconnect_reason_name(info->disconnect_reason));
        if (info->raw_disconnect_reason != 0) {
            printf(" (%d)", info->raw_disconnect_reason);
        }
        printf("\n");
    }
}

void run_wifi_connect(void)
{
    blusys_wifi_t *wifi;
    blusys_wifi_ip_info_t ip;
    blusys_err_t err;

    blusys_wifi_sta_config_t cfg = {
        .ssid               = CONFIG_WIFI_SSID,
        .password           = CONFIG_WIFI_PASSWORD,
        .auto_reconnect     = true,
        .reconnect_delay_ms = 1000,
        .max_retries        = -1,
        .on_event           = on_wifi_event,
    };

    err = blusys_wifi_open(&cfg, &wifi);
    if (err != BLUSYS_OK) {
        printf("wifi_open failed: %s\n", blusys_err_string(err));
        return;
    }

    printf("Connecting to '%s'...\n", CONFIG_WIFI_SSID);

    err = blusys_wifi_connect(wifi, CONFIG_WIFI_CONNECT_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        blusys_wifi_disconnect_reason_t reason = blusys_wifi_get_last_disconnect_reason(wifi);
        int raw_reason = blusys_wifi_get_last_disconnect_reason_raw(wifi);

        printf("wifi_connect failed: %s\n", blusys_err_string(err));
        printf("last disconnect reason: %s", disconnect_reason_name(reason));
        if (raw_reason != 0) {
            printf(" (%d)", raw_reason);
        }
        printf("\n");
        blusys_wifi_close(wifi);
        return;
    }

    err = blusys_wifi_get_ip_info(wifi, &ip);
    if (err == BLUSYS_OK) {
        printf("Connected.\n");
        printf("  IP:      %s\n", ip.ip);
        printf("  Netmask: %s\n", ip.netmask);
        printf("  Gateway: %s\n", ip.gateway);
    }

    vTaskDelay(pdMS_TO_TICKS(3000));

    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);

    printf("Done.\n");
}

#else /* !CONFIG_CONNECTIVITY_SCENARIO_WIFI_CONNECT */

void run_wifi_connect(void) {}

#endif /* CONFIG_CONNECTIVITY_SCENARIO_WIFI_CONNECT */
