#pragma once

#include "blusys/hal/error.h"

#include <stdbool.h>

#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool nvs_ready;
    bool netif_ready;
    bool event_loop_ready;
    bool wifi_ready;
} blusys_net_bootstrap_t;

blusys_err_t blusys_net_bootstrap_start(blusys_net_bootstrap_t *bootstrap);
blusys_err_t blusys_net_bootstrap_start_wifi(blusys_net_bootstrap_t *bootstrap);
void         blusys_net_bootstrap_stop(blusys_net_bootstrap_t *bootstrap);

esp_netif_t  *blusys_net_bootstrap_create_wifi_sta(void);
esp_netif_t  *blusys_net_bootstrap_create_wifi_ap(void);
blusys_err_t  blusys_net_bootstrap_create_mesh_netifs(esp_netif_t **out_sta,
                                                      esp_netif_t **out_ap);

#ifdef __cplusplus
}
#endif
