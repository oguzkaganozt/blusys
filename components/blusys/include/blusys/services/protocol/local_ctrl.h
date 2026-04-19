#ifndef BLUSYS_LOCAL_CTRL_H
#define BLUSYS_LOCAL_CTRL_H

#include <stdbool.h>

#include "blusys/services/protocol/local_ctrl_fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct blusys_local_ctrl_action {
    const char *name;    /* required; URL-safe action name, e.g. "restart" */
    const char *label;   /* optional; display label for the built-in page */
    blusys_local_ctrl_action_cb_t handler;
};

struct blusys_local_ctrl_config {
    const char *device_name;   /* required; shown on the built-in page */
    uint16_t http_port;        /* 0 = default 80 */
    const blusys_local_ctrl_action_t *actions;
    size_t action_count;
    size_t max_body_len;       /* 0 = default 256 bytes */
    size_t max_response_len;   /* 0 = default 512 bytes */
    blusys_local_ctrl_status_cb_t status_cb; /* optional; enables GET /api/status */
    void *user_ctx;            /* passed to callbacks */
};

blusys_err_t blusys_local_ctrl_open(const blusys_local_ctrl_config_t *config,
                                    blusys_local_ctrl_t **out_ctrl);
blusys_err_t blusys_local_ctrl_close(blusys_local_ctrl_t *ctrl);
bool blusys_local_ctrl_is_running(blusys_local_ctrl_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif
