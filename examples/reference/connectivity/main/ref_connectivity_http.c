#include <stdio.h>
#include <stdlib.h>

#include "sdkconfig.h"
#include "blusys/blusys.h"

#if CONFIG_CONNECTIVITY_SCENARIO_HTTP_CLIENT

void run_http_client_basic(void)
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

    printf("Connected.\n");

    /* Open HTTP client */
    blusys_http_client_t *http;
    blusys_http_client_config_t http_cfg = {
        .url          = CONFIG_HTTP_CLIENT_URL,
        .cert_pem     = NULL,
        .timeout_ms   = CONFIG_HTTP_CLIENT_TIMEOUT_MS,
        .max_response_body_size = 4096,
    };

    err = blusys_http_client_open(&http_cfg, &http);
    if (err != BLUSYS_OK) {
        printf("http_client_open failed: %s\n", blusys_err_string(err));
        blusys_wifi_close(wifi);
        return;
    }

    /* Perform GET request */
    printf("GET %s\n", CONFIG_HTTP_CLIENT_URL);

    blusys_http_response_t response;
    err = blusys_http_client_get(http, NULL, &response);
    if (err != BLUSYS_OK) {
        printf("http_client_get failed: %s\n", blusys_err_string(err));
    } else {
        printf("Status: %d\n", response.status_code);
        printf("Body (%zu bytes):\n%s\n",
               response.body_size,
               response.body ? (char *)response.body : "(empty)");
        if (response.body_truncated) {
            printf("(response body was truncated)\n");
        }
        free(response.body);
    }

    blusys_http_client_close(http);
    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);

    printf("Done.\n");
}

#else /* !CONFIG_CONNECTIVITY_SCENARIO_HTTP_CLIENT */

void run_http_client_basic(void) {}

#endif /* CONFIG_CONNECTIVITY_SCENARIO_HTTP_CLIENT */
