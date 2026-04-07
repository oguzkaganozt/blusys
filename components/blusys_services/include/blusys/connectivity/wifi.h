#ifndef BLUSYS_WIFI_H
#define BLUSYS_WIFI_H

#include <stdbool.h>
#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_wifi blusys_wifi_t;

typedef struct {
    const char *ssid;       /* AP SSID (required) */
    const char *password;   /* AP password, NULL or "" for open networks */
} blusys_wifi_sta_config_t;

typedef struct {
    char ip[16];       /* dotted-decimal string, e.g. "192.168.1.100" */
    char netmask[16];
    char gateway[16];
} blusys_wifi_ip_info_t;

blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config, blusys_wifi_t **out_wifi);
blusys_err_t blusys_wifi_close(blusys_wifi_t *wifi);
blusys_err_t blusys_wifi_connect(blusys_wifi_t *wifi, int timeout_ms);
blusys_err_t blusys_wifi_disconnect(blusys_wifi_t *wifi);
blusys_err_t blusys_wifi_get_ip_info(blusys_wifi_t *wifi, blusys_wifi_ip_info_t *out_info);
bool         blusys_wifi_is_connected(blusys_wifi_t *wifi);

#ifdef __cplusplus
}
#endif

#endif
