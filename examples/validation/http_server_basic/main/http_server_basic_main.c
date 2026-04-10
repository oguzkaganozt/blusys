#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys_services.h"

/* GET / — returns a plain-text greeting */
static blusys_err_t handle_root(blusys_http_server_req_t *req, void *user_ctx)
{
    (void) user_ctx;
    return blusys_http_server_resp_send(req, 200, "text/plain", "Hello from blusys!", -1);
}

/* POST /echo — reads the request body and echoes it back */
static blusys_err_t handle_echo(blusys_http_server_req_t *req, void *user_ctx)
{
    (void) user_ctx;

    size_t content_len = blusys_http_server_req_content_len(req);
    if (content_len == 0) {
        return blusys_http_server_resp_send(req, 200, "text/plain", "", 0);
    }

    char *buf = malloc(content_len + 1);
    if (buf == NULL) {
        return blusys_http_server_resp_send(req, 500, "text/plain", "Out of memory", -1);
    }

    size_t total = 0;
    while (total < content_len) {
        size_t chunk = 0;
        blusys_err_t err = blusys_http_server_req_recv(req, (uint8_t *)buf + total,
                                                        content_len - total, &chunk);
        if (err != BLUSYS_OK) {
            free(buf);
            return err;
        }
        if (chunk == 0) {
            break;
        }
        total += chunk;
    }
    buf[total] = '\0';

    blusys_err_t err = blusys_http_server_resp_send(req, 200, "text/plain", buf, (int)total);
    free(buf);
    return err;
}

static const blusys_http_server_route_t routes[] = {
    { .uri = "/",     .method = BLUSYS_HTTP_METHOD_GET,  .handler = handle_root },
    { .uri = "/echo", .method = BLUSYS_HTTP_METHOD_POST, .handler = handle_echo },
};

void app_main(void)
{
    /* Connect to WiFi */
    blusys_wifi_sta_config_t wifi_cfg = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    blusys_wifi_t *wifi = NULL;
    blusys_err_t err = blusys_wifi_open(&wifi_cfg, &wifi);
    if (err != BLUSYS_OK) {
        printf("WiFi open failed: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_wifi_connect(wifi, CONFIG_WIFI_CONNECT_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        printf("WiFi connect failed: %s\n", blusys_err_string(err));
        blusys_wifi_close(wifi);
        return;
    }

    blusys_wifi_ip_info_t ip_info;
    err = blusys_wifi_get_ip_info(wifi, &ip_info);
    if (err != BLUSYS_OK) {
        printf("WiFi get IP failed: %s\n", blusys_err_string(err));
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }
    printf("WiFi connected. IP: %s\n", ip_info.ip);

    /* Start HTTP server */
    blusys_http_server_config_t srv_cfg = {
        .port        = CONFIG_HTTP_SERVER_PORT,
        .routes      = routes,
        .route_count = sizeof(routes) / sizeof(routes[0]),
    };
    blusys_http_server_t *server = NULL;
    err = blusys_http_server_open(&srv_cfg, &server);
    if (err != BLUSYS_OK) {
        printf("HTTP server open failed: %s\n", blusys_err_string(err));
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }

    printf("HTTP server running on port %d\n", CONFIG_HTTP_SERVER_PORT);
    printf("  curl http://%s:%d/\n", ip_info.ip, CONFIG_HTTP_SERVER_PORT);
    printf("  curl -X POST http://%s:%d/echo -d \"hello\"\n", ip_info.ip, CONFIG_HTTP_SERVER_PORT);

    /* Run indefinitely — server handles requests in the background */
    vTaskDelay(portMAX_DELAY);
}
