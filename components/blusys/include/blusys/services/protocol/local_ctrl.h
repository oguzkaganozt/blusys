/**
 * @file local_ctrl.h
 * @brief Minimal HTTP server exposing a built-in control page plus `/api/*` routes.
 *
 * Purpose-built for simple LAN control use cases (reboot the device, read
 * status as JSON, trigger named actions). For anything richer, use
 * ::blusys_http_server directly. See docs/services/local_ctrl.md.
 */

#ifndef BLUSYS_LOCAL_CTRL_H
#define BLUSYS_LOCAL_CTRL_H

#include <stdbool.h>

#include "blusys/services/protocol/local_ctrl_fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief One user-defined action exposed as `POST /api/action/<name>`. */
struct blusys_local_ctrl_action {
    const char                    *name;    /**< Required URL-safe action name, e.g. `"restart"`. */
    const char                    *label;   /**< Optional display label for the built-in page. */
    blusys_local_ctrl_action_cb_t  handler; /**< Handler callback. */
};

/** @brief Configuration for ::blusys_local_ctrl_open. */
struct blusys_local_ctrl_config {
    const char                       *device_name;       /**< Required name shown on the built-in page. */
    uint16_t                          http_port;         /**< Listen port; `0` selects 80. */
    const blusys_local_ctrl_action_t *actions;           /**< Action table. */
    size_t                            action_count;      /**< Number of actions. */
    size_t                            max_body_len;      /**< Max request body size; `0` selects 256 bytes. */
    size_t                            max_response_len;  /**< Max response body size; `0` selects 512 bytes. */
    blusys_local_ctrl_status_cb_t     status_cb;         /**< Optional status callback; enables `GET /api/status`. */
    void                             *user_ctx;          /**< Opaque pointer forwarded to both callbacks. */
};

/**
 * @brief Start the local-control HTTP server.
 * @param config    Configuration.
 * @param out_ctrl  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_local_ctrl_open(const blusys_local_ctrl_config_t *config,
                                    blusys_local_ctrl_t **out_ctrl);

/**
 * @brief Stop the server and free the handle.
 * @param ctrl  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p ctrl is NULL.
 */
blusys_err_t blusys_local_ctrl_close(blusys_local_ctrl_t *ctrl);

/** @brief Whether the server is currently running. */
bool blusys_local_ctrl_is_running(blusys_local_ctrl_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif
