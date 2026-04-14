#ifndef BLUSYS_SNTP_H
#define BLUSYS_SNTP_H

#include <stdbool.h>
#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_sntp blusys_sntp_t;

typedef void (*blusys_sntp_sync_cb_t)(void *user_ctx);

typedef struct {
    const char             *server;       /**< Primary NTP server (default: "pool.ntp.org") */
    const char             *server2;      /**< Optional secondary server (NULL to skip) */
    bool                    smooth_sync;  /**< true = gradual adjustment, false = immediate jump */
    blusys_sntp_sync_cb_t   sync_cb;      /**< Optional callback invoked on each sync event */
    void                   *user_ctx;     /**< User context passed to sync_cb */
} blusys_sntp_config_t;

blusys_err_t blusys_sntp_open(const blusys_sntp_config_t *config, blusys_sntp_t **out_sntp);
blusys_err_t blusys_sntp_close(blusys_sntp_t *sntp);
blusys_err_t blusys_sntp_sync(blusys_sntp_t *sntp, int timeout_ms);
bool         blusys_sntp_is_synced(blusys_sntp_t *sntp);

#ifdef __cplusplus
}
#endif

#endif
