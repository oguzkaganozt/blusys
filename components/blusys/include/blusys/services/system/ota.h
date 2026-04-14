#ifndef BLUSYS_OTA_H
#define BLUSYS_OTA_H

#include "blusys/hal/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_ota blusys_ota_t;

/** Optional download progress (0–100). May be invoked often during `blusys_ota_perform`. */
typedef void (*blusys_ota_progress_cb_t)(uint8_t percent, void *user_ctx);

typedef struct {
    const char *url;        /* firmware URL; required; HTTP or HTTPS */
    const char *cert_pem;   /* server CA certificate (PEM); NULL disables TLS verification */
    int         timeout_ms; /* per-operation network timeout; 0 = default (30 000 ms) */
    blusys_ota_progress_cb_t on_progress; /* optional; NULL to ignore */
    void        *progress_ctx;            /* passed to on_progress */
} blusys_ota_config_t;

/* Allocate an OTA session handle.
   url and cert_pem pointers must remain valid until blusys_ota_perform() returns.
   Returns BLUSYS_ERR_INVALID_ARG if config, out_handle, or config->url is NULL.
   Returns BLUSYS_ERR_NO_MEM on allocation failure.
   Returns BLUSYS_ERR_NOT_SUPPORTED on targets without WiFi. */
blusys_err_t blusys_ota_open(const blusys_ota_config_t *config, blusys_ota_t **out_handle);

/* Download firmware from the configured URL and write it to the next OTA partition.
   Blocks until the download and flash are complete or an error occurs.
   On success the new partition is staged as the next boot target.
   On failure the current firmware is unaffected. */
blusys_err_t blusys_ota_perform(blusys_ota_t *handle);

/* Free the OTA handle. Call after blusys_ota_perform() whether it succeeded or failed. */
blusys_err_t blusys_ota_close(blusys_ota_t *handle);

/* Restart the device to boot the new firmware. Call after a successful blusys_ota_close(). */
void blusys_ota_restart(void);

/* Mark the currently running firmware as valid, cancelling any pending rollback.
   Call once after booting into new firmware and verifying it works correctly. */
blusys_err_t blusys_ota_mark_valid(void);

#ifdef __cplusplus
}
#endif

#endif
