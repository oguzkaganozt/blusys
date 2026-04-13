#include <stdio.h>
#include <time.h>
#include "blusys/connectivity/wifi.h"
#include "blusys/device/sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "sntp_basic";

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

    /* ---- SNTP ---- */
    blusys_sntp_t *sntp = NULL;
    blusys_sntp_config_t sntp_cfg = {
        .server = CONFIG_SNTP_SERVER,
    };

    ESP_ERROR_CHECK(blusys_sntp_open(&sntp_cfg, &sntp));
    ESP_LOGI(TAG, "Waiting for time sync...");

    blusys_err_t err = blusys_sntp_sync(sntp, CONFIG_SNTP_SYNC_TIMEOUT_MS);
    if (err == BLUSYS_OK) {
        ESP_LOGI(TAG, "Time synchronized");
    } else {
        ESP_LOGW(TAG, "Time sync failed (err=%d)", err);
    }

    /* Print current time */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "Current UTC time: %s", buf);
    ESP_LOGI(TAG, "Synced: %s", blusys_sntp_is_synced(sntp) ? "yes" : "no");

    /* ---- Cleanup ---- */
    blusys_sntp_close(sntp);
    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);
}
