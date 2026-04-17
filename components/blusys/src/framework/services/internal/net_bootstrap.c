#include "blusys/framework/services/internal/net_bootstrap.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/nvs_init.h"

blusys_err_t blusys_net_bootstrap_start(blusys_net_bootstrap_t *bootstrap)
{
    if (bootstrap == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        return err;
    }
    bootstrap->nvs_ready = true;

    esp_err_t esp_err = esp_netif_init();
    if (esp_err != ESP_OK) {
        blusys_net_bootstrap_stop(bootstrap);
        return blusys_translate_esp_err(esp_err);
    }
    bootstrap->netif_ready = true;

    esp_err = esp_event_loop_create_default();
    if (esp_err != ESP_OK) {
        blusys_net_bootstrap_stop(bootstrap);
        return blusys_translate_esp_err(esp_err);
    }
    bootstrap->event_loop_ready = true;

    return BLUSYS_OK;
}

blusys_err_t blusys_net_bootstrap_start_wifi(blusys_net_bootstrap_t *bootstrap)
{
    if (bootstrap == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_net_bootstrap_start(bootstrap);
    if (err != BLUSYS_OK) {
        return err;
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t esp_err = esp_wifi_init(&wifi_cfg);
    if (esp_err != ESP_OK) {
        blusys_net_bootstrap_stop(bootstrap);
        return blusys_translate_esp_err(esp_err);
    }

    bootstrap->wifi_ready = true;
    return BLUSYS_OK;
}

void blusys_net_bootstrap_stop(blusys_net_bootstrap_t *bootstrap)
{
    if (bootstrap == NULL) {
        return;
    }

    if (bootstrap->wifi_ready) {
        esp_wifi_deinit();
        bootstrap->wifi_ready = false;
    }

    if (bootstrap->event_loop_ready) {
        esp_event_loop_delete_default();
        bootstrap->event_loop_ready = false;
    }

    if (bootstrap->netif_ready) {
        esp_netif_deinit();
        bootstrap->netif_ready = false;
    }

    bootstrap->nvs_ready = false;
}

esp_netif_t *blusys_net_bootstrap_create_wifi_sta(void)
{
    return esp_netif_create_default_wifi_sta();
}

esp_netif_t *blusys_net_bootstrap_create_wifi_ap(void)
{
    return esp_netif_create_default_wifi_ap();
}

blusys_err_t blusys_net_bootstrap_create_mesh_netifs(esp_netif_t **out_sta,
                                                      esp_netif_t **out_ap)
{
    if (out_sta == NULL || out_ap == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err_t esp_err = esp_netif_create_default_wifi_mesh_netifs(out_sta, out_ap);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    return BLUSYS_OK;
}
