/**
 * @file sntp.h
 * @brief SNTP time sync against one or two NTP servers.
 *
 * Calls `settimeofday()` under the hood; use `gettimeofday()` / `localtime_r()`
 * to read the synced clock. See docs/services/sntp.md.
 */

#ifndef BLUSYS_SNTP_H
#define BLUSYS_SNTP_H

#include <stdbool.h>
#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque SNTP client handle. */
typedef struct blusys_sntp blusys_sntp_t;

/**
 * @brief Sync-completion callback.
 *
 * Runs on the LWIP task context; keep it short.
 *
 * @param user_ctx  User pointer supplied via ::blusys_sntp_config_t::user_ctx.
 */
typedef void (*blusys_sntp_sync_cb_t)(void *user_ctx);

/** @brief Configuration for ::blusys_sntp_open. */
typedef struct {
    const char             *server;       /**< Primary NTP server; NULL selects `"pool.ntp.org"`. */
    const char             *server2;      /**< Optional secondary server; NULL to skip. */
    bool                    smooth_sync;  /**< `true` for gradual adjustment, `false` for immediate step. */
    blusys_sntp_sync_cb_t   sync_cb;      /**< Optional callback invoked on each sync event. */
    void                   *user_ctx;     /**< User context passed to @ref sync_cb. */
} blusys_sntp_config_t;

/**
 * @brief Start the SNTP client.
 * @param config    Configuration.
 * @param out_sntp  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_sntp_open(const blusys_sntp_config_t *config, blusys_sntp_t **out_sntp);

/**
 * @brief Stop the SNTP client and free the handle.
 * @param sntp  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p sntp is NULL.
 */
blusys_err_t blusys_sntp_close(blusys_sntp_t *sntp);

/**
 * @brief Block until the next sync completes, up to @p timeout_ms.
 * @param sntp        Handle.
 * @param timeout_ms  Timeout in ms; use `BLUSYS_TIMEOUT_FOREVER` to block.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`.
 */
blusys_err_t blusys_sntp_sync(blusys_sntp_t *sntp, int timeout_ms);

/**
 * @brief Whether the system clock has been synced at least once.
 * @param sntp  Handle.
 */
bool blusys_sntp_is_synced(blusys_sntp_t *sntp);

#ifdef __cplusplus
}
#endif

#endif
