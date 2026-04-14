#ifndef BLUSYS_LOCAL_CTRL_H
#define BLUSYS_LOCAL_CTRL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_local_ctrl blusys_local_ctrl_t;

typedef blusys_err_t (*blusys_local_ctrl_status_cb_t)(char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      void *user_ctx);

typedef blusys_err_t (*blusys_local_ctrl_action_cb_t)(const char *action_name,
                                                      const uint8_t *body,
                                                      size_t body_len,
                                                      char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      int *out_status_code,
                                                      void *user_ctx);

typedef struct {
    const char *name;    /* required; URL-safe action name, e.g. "restart" */
    const char *label;   /* optional; display label for the built-in page */
    blusys_local_ctrl_action_cb_t handler;
} blusys_local_ctrl_action_t;

typedef struct {
    const char *device_name;   /* required; shown on the built-in page */
    uint16_t http_port;        /* 0 = default 80 */
    const blusys_local_ctrl_action_t *actions;
    size_t action_count;
    size_t max_body_len;       /* 0 = default 256 bytes */
    size_t max_response_len;   /* 0 = default 512 bytes */
    blusys_local_ctrl_status_cb_t status_cb; /* optional; enables GET /api/status */
    void *user_ctx;            /* passed to callbacks */
} blusys_local_ctrl_config_t;

blusys_err_t blusys_local_ctrl_open(const blusys_local_ctrl_config_t *config,
                                    blusys_local_ctrl_t **out_ctrl);
blusys_err_t blusys_local_ctrl_close(blusys_local_ctrl_t *ctrl);
bool blusys_local_ctrl_is_running(blusys_local_ctrl_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif
