#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

typedef struct {
    bool output_enabled;
    int action_count;
} app_state_t;

static blusys_err_t build_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{
    app_state_t *state = user_ctx;
    int written = snprintf(json_buf,
                           buf_len,
                           "{\"output_enabled\":%s,\"action_count\":%d}",
                           state->output_enabled ? "true" : "false",
                           state->action_count);
    if ((written < 0) || ((size_t)written >= buf_len)) {
        return BLUSYS_ERR_NO_MEM;
    }

    *out_len = (size_t)written;
    return BLUSYS_OK;
}

static blusys_err_t handle_action(const char *action_name,
                                  const uint8_t *body,
                                  size_t body_len,
                                  char *json_buf,
                                  size_t buf_len,
                                  size_t *out_len,
                                  int *out_status_code,
                                  void *user_ctx)
{
    (void)body;
    (void)body_len;

    app_state_t *state = user_ctx;

    if (strcmp(action_name, "enable") == 0) {
        state->output_enabled = true;
    } else if (strcmp(action_name, "disable") == 0) {
        state->output_enabled = false;
    } else if (strcmp(action_name, "toggle") == 0) {
        state->output_enabled = !state->output_enabled;
    } else {
        int written = snprintf(json_buf,
                               buf_len,
                               "{\"ok\":false,\"error\":\"unknown_action\"}");
        if ((written < 0) || ((size_t)written >= buf_len)) {
            return BLUSYS_ERR_NO_MEM;
        }

        *out_status_code = 404;
        *out_len = (size_t)written;
        return BLUSYS_OK;
    }

    state->action_count++;

    int written = snprintf(json_buf,
                           buf_len,
                           "{\"ok\":true,\"action\":\"%s\",\"output_enabled\":%s,\"action_count\":%d}",
                           action_name,
                           state->output_enabled ? "true" : "false",
                           state->action_count);
    if ((written < 0) || ((size_t)written >= buf_len)) {
        return BLUSYS_ERR_NO_MEM;
    }

    *out_status_code = 200;
    *out_len = (size_t)written;
    return BLUSYS_OK;
}

static const blusys_local_ctrl_action_t actions[] = {
    { .name = "enable",  .label = "Enable Output",  .handler = handle_action },
    { .name = "disable", .label = "Disable Output", .handler = handle_action },
    { .name = "toggle",  .label = "Toggle Output",  .handler = handle_action },
};

void app_main(void)
{
    app_state_t state = {
        .output_enabled = false,
        .action_count = 0,
    };

    blusys_wifi_sta_config_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
        .auto_reconnect = true,
        .reconnect_delay_ms = 1000,
        .max_retries = -1,
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
        blusys_wifi_close(wifi);
        return;
    }

    blusys_local_ctrl_config_t ctrl_cfg = {
        .device_name = CONFIG_LOCAL_CTRL_DEVICE_NAME,
        .http_port = CONFIG_LOCAL_CTRL_HTTP_PORT,
        .actions = actions,
        .action_count = sizeof(actions) / sizeof(actions[0]),
        .status_cb = build_status,
        .user_ctx = &state,
    };

    blusys_local_ctrl_t *ctrl = NULL;
    err = blusys_local_ctrl_open(&ctrl_cfg, &ctrl);
    if (err != BLUSYS_OK) {
        printf("local_ctrl open failed: %s\n", blusys_err_string(err));
        blusys_wifi_close(wifi);
        return;
    }

    printf("Local control ready\n");
    printf("  Browser: http://%s:%d/\n", ip_info.ip, CONFIG_LOCAL_CTRL_HTTP_PORT);
    printf("  Info API: http://%s:%d/api/info\n", ip_info.ip, CONFIG_LOCAL_CTRL_HTTP_PORT);
    printf("  Status API: http://%s:%d/api/status\n", ip_info.ip, CONFIG_LOCAL_CTRL_HTTP_PORT);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        printf("local_ctrl running=%s output_enabled=%s action_count=%d\n",
               blusys_local_ctrl_is_running(ctrl) ? "true" : "false",
               state.output_enabled ? "true" : "false",
               state.action_count);
    }
}
