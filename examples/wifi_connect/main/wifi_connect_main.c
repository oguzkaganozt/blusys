#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_wifi_t *wifi;
    blusys_wifi_ip_info_t ip;
    blusys_err_t err;

    blusys_wifi_sta_config_t cfg = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };

    err = blusys_wifi_open(&cfg, &wifi);
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
