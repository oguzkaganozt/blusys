#ifndef BLUSYS_WIFI_H
#define BLUSYS_WIFI_H

#include <stdbool.h>
#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_wifi blusys_wifi_t;

typedef enum {
    BLUSYS_WIFI_EVENT_STARTED = 0,
    BLUSYS_WIFI_EVENT_CONNECTING,
    BLUSYS_WIFI_EVENT_CONNECTED,
    BLUSYS_WIFI_EVENT_GOT_IP,
    BLUSYS_WIFI_EVENT_DISCONNECTED,
    BLUSYS_WIFI_EVENT_RECONNECTING,
    BLUSYS_WIFI_EVENT_STOPPED,
} blusys_wifi_event_t;

typedef enum {
    BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN = 0,
    BLUSYS_WIFI_DISCONNECT_REASON_USER_REQUESTED,
    BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED,
    BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND,
    BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED,
    BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST,
} blusys_wifi_disconnect_reason_t;

typedef struct {
    char ip[16];       /* dotted-decimal string, e.g. "192.168.1.100" */
    char netmask[16];
    char gateway[16];
} blusys_wifi_ip_info_t;

typedef struct {
    blusys_wifi_disconnect_reason_t disconnect_reason;
    int                             retry_attempt;
    blusys_wifi_ip_info_t           ip_info;
} blusys_wifi_event_info_t;

typedef void (*blusys_wifi_event_cb_t)(blusys_wifi_t *wifi,
                                        blusys_wifi_event_t event,
                                        const blusys_wifi_event_info_t *info,
                                        void *user_ctx);

typedef struct {
    const char              *ssid;               /* AP SSID (required) */
    const char              *password;           /* AP password, NULL or "" for open networks */
    bool                     auto_reconnect;     /* reconnect after unexpected disconnect */
    int                      reconnect_delay_ms; /* delay before reconnect; <= 0 uses default */
    int                      max_retries;        /* 0 disables retries, -1 retries forever */
    blusys_wifi_event_cb_t   on_event;           /* optional async lifecycle callback */
    void                    *user_ctx;           /* passed back in on_event */
} blusys_wifi_sta_config_t;

blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config, blusys_wifi_t **out_wifi);
blusys_err_t blusys_wifi_close(blusys_wifi_t *wifi);
blusys_err_t blusys_wifi_connect(blusys_wifi_t *wifi, int timeout_ms);
blusys_err_t blusys_wifi_disconnect(blusys_wifi_t *wifi);
blusys_err_t blusys_wifi_get_ip_info(blusys_wifi_t *wifi, blusys_wifi_ip_info_t *out_info);
blusys_wifi_disconnect_reason_t blusys_wifi_get_last_disconnect_reason(blusys_wifi_t *wifi);
bool         blusys_wifi_is_connected(blusys_wifi_t *wifi);

#ifdef __cplusplus
}
#endif

#endif
