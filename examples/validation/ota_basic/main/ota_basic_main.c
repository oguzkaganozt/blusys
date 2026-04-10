#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys_services.h"

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

    /* Download and flash firmware */
    blusys_ota_config_t ota_cfg = {
        .url = CONFIG_OTA_FIRMWARE_URL,
    };
    blusys_ota_t *ota = NULL;
    err = blusys_ota_open(&ota_cfg, &ota);
    if (err != BLUSYS_OK) {
        printf("OTA open failed: %s\n", blusys_err_string(err));
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }

    printf("Downloading firmware from %s ...\n", CONFIG_OTA_FIRMWARE_URL);
    err = blusys_ota_perform(ota);
    blusys_ota_close(ota);

    if (err != BLUSYS_OK) {
        printf("OTA failed: %s\n", blusys_err_string(err));
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        vTaskDelay(portMAX_DELAY);
        return;
    }

    printf("OTA complete. Restarting...\n");
    blusys_ota_restart();
}
