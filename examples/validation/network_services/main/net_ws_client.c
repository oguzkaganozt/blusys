#if CONFIG_NETWORK_SERVICES_SCENARIO_WS_CLIENT
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "blusys/blusys.h"

static void on_message(blusys_ws_msg_type_t type,
                        const uint8_t *data, size_t len,
                        void *user_ctx)
{
    (void) user_ctx;
    const char *type_str = (type == BLUSYS_WS_MSG_TEXT) ? "text" : "binary";
    printf("Received [%s, %u bytes]: %.*s\n",
           type_str, (unsigned)len, (int)len, (const char *)data);
}

static void on_disconnect(void *user_ctx)
{
    (void) user_ctx;
    printf("WebSocket disconnected.\n");
}

void run_net_ws_client(void)
{
    blusys_err_t err;

    /* Connect to WiFi */
    blusys_wifi_t *wifi;
    blusys_wifi_sta_config_t wifi_cfg = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };

    err = blusys_wifi_open(&wifi_cfg, &wifi);
    if (err != BLUSYS_OK) {
        printf("wifi_open failed: %s\n", blusys_err_string(err));
        return;
    }

    printf("Connecting to '%s'...\n", CONFIG_WIFI_SSID);

    err = blusys_wifi_connect(wifi, CONFIG_WIFI_CONNECT_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        printf("wifi_connect failed: %s\n", blusys_err_string(err));
        blusys_wifi_close(wifi);
        return;
    }

    printf("WiFi connected.\n");

    /* Open WebSocket client */
    blusys_ws_client_t *ws;
    blusys_ws_client_config_t ws_cfg = {
        .url           = CONFIG_WS_SERVER_URL,
        .timeout_ms    = CONFIG_WS_CONNECT_TIMEOUT_MS,
        .message_cb    = on_message,
        .disconnect_cb = on_disconnect,
        .user_ctx      = NULL,
    };

    err = blusys_ws_client_open(&ws_cfg, &ws);
    if (err != BLUSYS_OK) {
        printf("ws_client_open failed: %s\n", blusys_err_string(err));
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }

    /* Connect to server */
    printf("Connecting to '%s'...\n", CONFIG_WS_SERVER_URL);

    err = blusys_ws_client_connect(ws);
    if (err != BLUSYS_OK) {
        printf("ws_client_connect failed: %s\n", blusys_err_string(err));
        blusys_ws_client_close(ws);
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }

    printf("WebSocket connected.\n");

    /* Send a message every CONFIG_WS_SEND_INTERVAL_MS — the echo server
       will return it and trigger the on_message callback. */
    int count = 0;
    while (blusys_ws_client_is_connected(ws)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "hello from blusys #%d", count++);

        err = blusys_ws_client_send_text(ws, msg);
        if (err != BLUSYS_OK) {
            printf("send_text failed: %s\n", blusys_err_string(err));
            break;
        }

        printf("Sent: %s\n", msg);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_WS_SEND_INTERVAL_MS));
    }

    /* Cleanup */
    blusys_ws_client_disconnect(ws);
    blusys_ws_client_close(ws);
    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);

    printf("Done.\n");
}

#else /* !CONFIG_NETWORK_SERVICES_SCENARIO_WS_CLIENT */
void run_net_ws_client(void) {}
#endif /* CONFIG_NETWORK_SERVICES_SCENARIO_WS_CLIENT */
