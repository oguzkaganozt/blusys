#include "sdkconfig.h"

#if CONFIG_WIRELESS_ESP_LAB_SCENARIO_WIFI_MESH_BASIC

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys.h"


static const char *event_name(blusys_wifi_mesh_event_t event)
{
    switch (event) {
        case BLUSYS_WIFI_MESH_EVENT_STARTED:             return "started";
        case BLUSYS_WIFI_MESH_EVENT_STOPPED:             return "stopped";
        case BLUSYS_WIFI_MESH_EVENT_PARENT_CONNECTED:    return "parent_connected";
        case BLUSYS_WIFI_MESH_EVENT_PARENT_DISCONNECTED: return "parent_disconnected";
        case BLUSYS_WIFI_MESH_EVENT_CHILD_CONNECTED:     return "child_connected";
        case BLUSYS_WIFI_MESH_EVENT_CHILD_DISCONNECTED:  return "child_disconnected";
        case BLUSYS_WIFI_MESH_EVENT_NO_PARENT_FOUND:     return "no_parent_found";
        case BLUSYS_WIFI_MESH_EVENT_GOT_IP:              return "got_ip";
        default:                                          return "unknown";
    }
}

static void on_mesh_event(blusys_wifi_mesh_t *mesh,
                           blusys_wifi_mesh_event_t event,
                           const blusys_wifi_mesh_event_info_t *info,
                           void *user_ctx)
{
    (void)mesh;
    (void)user_ctx;

    printf("[mesh] event: %s\n", event_name(event));

    if (info != NULL) {
        const uint8_t *a = info->peer.addr;
        if ((event == BLUSYS_WIFI_MESH_EVENT_PARENT_CONNECTED) ||
            (event == BLUSYS_WIFI_MESH_EVENT_PARENT_DISCONNECTED)) {
            printf("[mesh] parent: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   a[0], a[1], a[2], a[3], a[4], a[5]);
        }
        if ((event == BLUSYS_WIFI_MESH_EVENT_CHILD_CONNECTED) ||
            (event == BLUSYS_WIFI_MESH_EVENT_CHILD_DISCONNECTED)) {
            printf("[mesh] child:  %02x:%02x:%02x:%02x:%02x:%02x\n",
                   a[0], a[1], a[2], a[3], a[4], a[5]);
        }
    }
}

/* Parse CONFIG_MESH_ID string "77:77:77:77:77:77" into 6 bytes */
static void parse_mesh_id(const char *str, uint8_t out[6])
{
    unsigned v[6] = {0};
    int matched = sscanf(str, "%x:%x:%x:%x:%x:%x",
                         &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
    if (matched != 6) {
        printf("[mesh] WARNING: CONFIG_MESH_ID \"%s\" is malformed, using zeros\n", str);
    }
    for (int i = 0; i < 6; i++) {
        out[i] = (uint8_t)v[i];
    }
}

void run_wifi_mesh_basic(void)
{
    blusys_wifi_mesh_t *mesh = NULL;

    uint8_t mesh_id[6];
    parse_mesh_id(CONFIG_MESH_ID, mesh_id);

    blusys_wifi_mesh_config_t cfg = {
        .password        = CONFIG_MESH_PASSWORD,
        .channel         = CONFIG_MESH_CHANNEL,
        .router_ssid     = CONFIG_MESH_ROUTER_SSID,
        .router_password = CONFIG_MESH_ROUTER_PASSWORD,
        .max_layer       = CONFIG_MESH_MAX_LAYER,
        .max_connections = CONFIG_MESH_MAX_CONNECTIONS,
        .on_event        = on_mesh_event,
    };
    memcpy(cfg.mesh_id, mesh_id, 6);

    printf("[mesh] opening mesh network (ID %s)...\n", CONFIG_MESH_ID);

    blusys_err_t err = blusys_wifi_mesh_open(&cfg, &mesh);
    if (err != BLUSYS_OK) {
        printf("[mesh] open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("[mesh] open OK\n");

    /* Run for 30 s, polling layer and root status every 5 s */
    for (int i = 0; i < 6; i++) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        int layer = 0;
        bool is_root = blusys_wifi_mesh_is_root(mesh);
        blusys_wifi_mesh_get_layer(mesh, &layer);
        printf("[mesh] layer=%d  root=%s\n", layer, is_root ? "yes" : "no");
    }

    printf("[mesh] closing...\n");
    err = blusys_wifi_mesh_close(mesh);
    printf("[mesh] close: %s\n", err == BLUSYS_OK ? "OK" : blusys_err_string(err));
}


#else

void run_wifi_mesh_basic(void) {}

#endif /* CONFIG_WIRELESS_ESP_LAB_SCENARIO_WIFI_MESH_BASIC */
