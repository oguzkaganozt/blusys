#include <stdio.h>
#include "blusys/services/connectivity/wifi.h"
#include "blusys/services/protocol/mdns.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mdns_basic";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    /* ---- Wi-Fi ---- */
    blusys_wifi_t *wifi = NULL;
    blusys_wifi_sta_config_t wifi_cfg = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };

    ESP_ERROR_CHECK(blusys_wifi_open(&wifi_cfg, &wifi));
    ESP_ERROR_CHECK(blusys_wifi_connect(wifi, 10000));
    ESP_LOGI(TAG, "Wi-Fi connected");

    /* ---- mDNS ---- */
    blusys_mdns_t *mdns = NULL;
    blusys_mdns_config_t mdns_cfg = {
        .hostname      = CONFIG_MDNS_HOSTNAME,
        .instance_name = CONFIG_MDNS_INSTANCE,
    };

    ESP_ERROR_CHECK(blusys_mdns_open(&mdns_cfg, &mdns));
    ESP_LOGI(TAG, "mDNS started: %s.local", CONFIG_MDNS_HOSTNAME);

    /* Advertise an HTTP service on port 80 */
    blusys_mdns_txt_item_t txt[] = {
        { .key = "board", .value = "esp32" },
        { .key = "path",  .value = "/"     },
    };
    ESP_ERROR_CHECK(blusys_mdns_add_service(mdns, CONFIG_MDNS_INSTANCE,
                                             "_http", BLUSYS_MDNS_PROTO_TCP,
                                             80, txt, 2));
    ESP_LOGI(TAG, "Advertised _http._tcp service on port 80");

    /* Query for other HTTP services on the network */
    blusys_mdns_result_t results[8];
    size_t count = 0;
    blusys_err_t err = blusys_mdns_query(mdns, "_http", BLUSYS_MDNS_PROTO_TCP,
                                          3000, results, 8, &count);
    if (err == BLUSYS_OK) {
        ESP_LOGI(TAG, "Found %u HTTP service(s):", (unsigned)count);
        for (size_t i = 0; i < count; i++) {
            ESP_LOGI(TAG, "  [%u] %s (%s) at %s:%u",
                     (unsigned)i, results[i].instance_name,
                     results[i].hostname, results[i].ip, results[i].port);
        }
    } else {
        ESP_LOGW(TAG, "mDNS query failed (err=%d)", err);
    }

    /* Keep running so the service stays advertised */
    ESP_LOGI(TAG, "Device reachable at %s.local — press reset to stop", CONFIG_MDNS_HOSTNAME);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
