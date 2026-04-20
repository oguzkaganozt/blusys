/**
 * @file ota.h
 * @brief Over-the-air firmware download-and-flash helper.
 *
 * Downloads firmware from an HTTP/HTTPS URL, writes it to the next OTA
 * partition, optionally verifies a SHA256 digest, and stages the new image
 * as the next boot target. After booting into the new image, call
 * ::blusys_ota_mark_valid to cancel the pending rollback. See
 * docs/services/ota.md.
 */

#ifndef BLUSYS_OTA_H
#define BLUSYS_OTA_H

#include "blusys/hal/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque OTA session handle. */
typedef struct blusys_ota blusys_ota_t;

/**
 * @brief Optional download progress callback.
 *
 * Invoked during ::blusys_ota_perform on a background task context.
 *
 * @param percent   Download progress (0–100).
 * @param user_ctx  User pointer from ::blusys_ota_config_t::progress_ctx.
 */
typedef void (*blusys_ota_progress_cb_t)(uint8_t percent, void *user_ctx);

/** @brief Configuration for ::blusys_ota_open. */
typedef struct {
    const char *url;                        /**< Firmware URL (HTTP or HTTPS); required. */
    const char *cert_pem;                   /**< Server CA certificate (PEM); NULL disables TLS verification. */
    int         timeout_ms;                 /**< Per-operation network timeout; `0` selects 30 000 ms. */
    blusys_ota_progress_cb_t on_progress;   /**< Optional progress callback; may be NULL. */
    void       *progress_ctx;               /**< Opaque pointer passed to @p on_progress. */
    /**
     * @brief Optional SHA256 integrity check — 64 ASCII hex chars (any case).
     *
     * When non-NULL, the service reads the staged partition after flashing
     * and compares the digest. On mismatch the new partition is invalidated
     * and ::blusys_ota_perform returns `BLUSYS_ERR_INVALID_STATE`. NULL
     * skips verification.
     */
    const char *expected_sha256;
} blusys_ota_config_t;

/**
 * @brief Allocate an OTA session handle.
 *
 * The `url` and `cert_pem` pointers must remain valid until
 * ::blusys_ota_perform returns.
 *
 * @param config      Configuration.
 * @param out_handle  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any required field is
 *         NULL, `BLUSYS_ERR_NO_MEM`, `BLUSYS_ERR_NOT_SUPPORTED` on targets
 *         without WiFi.
 */
blusys_err_t blusys_ota_open(const blusys_ota_config_t *config, blusys_ota_t **out_handle);

/**
 * @brief Download firmware and flash it to the next OTA partition.
 *
 * Blocks until the download and flash complete or an error occurs. On
 * success the new partition is staged as the next boot target; on failure
 * the current firmware is unaffected.
 *
 * @param handle  Session handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` on SHA256 mismatch, or a
 *         translated network/flash error.
 */
blusys_err_t blusys_ota_perform(blusys_ota_t *handle);

/**
 * @brief Free the OTA handle.
 *
 * Call after ::blusys_ota_perform whether it succeeded or failed.
 *
 * @param handle  Session handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p handle is NULL.
 */
blusys_err_t blusys_ota_close(blusys_ota_t *handle);

/**
 * @brief Restart the device to boot the new firmware. Does not return.
 *
 * Call after a successful ::blusys_ota_close.
 */
void blusys_ota_restart(void);

/**
 * @brief Mark the currently running firmware as valid, cancelling any pending rollback.
 *
 * Call once after booting into new firmware and verifying it works
 * correctly. If the device resets before this is called, the bootloader
 * rolls back to the previous image.
 *
 * @return `BLUSYS_OK`, or a translated ESP-IDF error.
 */
blusys_err_t blusys_ota_mark_valid(void);

#ifdef __cplusplus
}
#endif

#endif
