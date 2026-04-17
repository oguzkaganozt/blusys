#include "blusys/services/system/ota.h"

#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include <ctype.h>
#include <string.h>

#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "mbedtls/sha256.h"

#include "blusys/hal/system.h"
#include "blusys/hal/internal/esp_err_shim.h"

#define DEFAULT_TIMEOUT_MS 30000
#define SHA256_BYTES       32
#define VERIFY_CHUNK_BYTES 4096

static const char *BLUSYS_OTA_TAG = "blusys_ota";

struct blusys_ota {
    const char *url;
    const char *cert_pem;
    int         timeout_ms;
    blusys_ota_progress_cb_t on_progress;
    void       *progress_ctx;
    const char *expected_sha256;
};

/* Decode 64 hex chars into 32 raw bytes. Returns true on success. */
static bool hex_decode_sha256(const char *hex, uint8_t out[SHA256_BYTES])
{
    if (hex == NULL) return false;
    for (size_t i = 0; i < SHA256_BYTES; ++i) {
        int hi = hex[2 * i];
        int lo = hex[2 * i + 1];
        if (!isxdigit(hi) || !isxdigit(lo)) return false;
        int h = (hi <= '9') ? (hi - '0') : ((hi | 0x20) - 'a' + 10);
        int l = (lo <= '9') ? (lo - '0') : ((lo | 0x20) - 'a' + 10);
        out[i] = (uint8_t)((h << 4) | l);
    }
    /* Reject trailing garbage: expect exactly 64 hex chars. */
    return hex[2 * SHA256_BYTES] == '\0';
}

/* Hash the first `len` bytes of `part` and compare to `expected`. */
static blusys_err_t verify_partition_sha256(const esp_partition_t *part,
                                             size_t                 len,
                                             const uint8_t          expected[SHA256_BYTES])
{
    if (part == NULL || len == 0) return BLUSYS_ERR_INVALID_ARG;

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    if (mbedtls_sha256_starts(&ctx, 0) != 0) {
        mbedtls_sha256_free(&ctx);
        return BLUSYS_ERR_INTERNAL;
    }

    uint8_t *buf = malloc(VERIFY_CHUNK_BYTES);
    if (buf == NULL) {
        mbedtls_sha256_free(&ctx);
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t rc = BLUSYS_OK;
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > VERIFY_CHUNK_BYTES) chunk = VERIFY_CHUNK_BYTES;
        esp_err_t err = esp_partition_read(part, offset, buf, chunk);
        if (err != ESP_OK) { rc = blusys_translate_esp_err(err); break; }
        if (mbedtls_sha256_update(&ctx, buf, chunk) != 0) { rc = BLUSYS_ERR_INTERNAL; break; }
        offset += chunk;
    }
    free(buf);

    if (rc == BLUSYS_OK) {
        uint8_t digest[SHA256_BYTES];
        if (mbedtls_sha256_finish(&ctx, digest) != 0) {
            rc = BLUSYS_ERR_INTERNAL;
        } else if (memcmp(digest, expected, SHA256_BYTES) != 0) {
            rc = BLUSYS_ERR_INVALID_STATE;
        }
    }

    mbedtls_sha256_free(&ctx);
    return rc;
}

blusys_err_t blusys_ota_open(const blusys_ota_config_t *config, blusys_ota_t **out_handle)
{
    if ((config == NULL) || (out_handle == NULL) || (config->url == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_ota_t *h = malloc(sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    h->url             = config->url;
    h->cert_pem        = config->cert_pem;
    h->timeout_ms      = config->timeout_ms;
    h->on_progress     = config->on_progress;
    h->progress_ctx    = config->progress_ctx;
    h->expected_sha256 = config->expected_sha256;

    *out_handle = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_ota_perform(blusys_ota_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_http_client_config_t http_cfg = {
        .url        = handle->url,
        .cert_pem   = handle->cert_pem,
        .timeout_ms = (handle->timeout_ms > 0) ? handle->timeout_ms : DEFAULT_TIMEOUT_MS,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t esp_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &esp_ota_handle);
    if (err != ESP_OK) {
        return blusys_translate_esp_err(err);
    }

    while (true) {
        err = esp_https_ota_perform(esp_ota_handle);

        if (handle->on_progress != NULL) {
            uint8_t pct = 0;
            if (err == ESP_OK) {
                pct = 100;
            } else {
                int total = esp_https_ota_get_image_size(esp_ota_handle);
                int read  = esp_https_ota_get_image_len_read(esp_ota_handle);
                if (total > 0 && read >= 0) {
                    unsigned long long num = (unsigned long long) read * 100ULL;
                    pct = (uint8_t)(num / (unsigned) total);
                    if (pct > 100) {
                        pct = 100;
                    }
                }
            }
            handle->on_progress(pct, handle->progress_ctx);
        }

        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
    }

    if (err != ESP_OK) {
        esp_https_ota_abort(esp_ota_handle);
        return blusys_translate_esp_err(err);
    }

    if (!esp_https_ota_is_complete_data_received(esp_ota_handle)) {
        esp_https_ota_abort(esp_ota_handle);
        return BLUSYS_ERR_IO;
    }

    /* Decode the expected digest (if any) before finishing, so a bogus
       checksum string is detected before we commit the new partition. */
    uint8_t expected[SHA256_BYTES];
    bool verify_requested = false;
    if (handle->expected_sha256 != NULL && handle->expected_sha256[0] != '\0') {
        if (!hex_decode_sha256(handle->expected_sha256, expected)) {
            ESP_LOGE(BLUSYS_OTA_TAG, "expected_sha256 is not 64 hex chars");
            esp_https_ota_abort(esp_ota_handle);
            return BLUSYS_ERR_INVALID_ARG;
        }
        verify_requested = true;
    }

    /* Snapshot the pending partition + its app image size before finish,
       since esp_https_ota frees internal state once finish() returns. */
    const esp_partition_t *staged = NULL;
    int image_size = 0;
    if (verify_requested) {
        staged     = esp_ota_get_next_update_partition(NULL);
        image_size = esp_https_ota_get_image_size(esp_ota_handle);
        if (staged == NULL || image_size <= 0) {
            esp_https_ota_abort(esp_ota_handle);
            return BLUSYS_ERR_INTERNAL;
        }
    }

    err = esp_https_ota_finish(esp_ota_handle);
    if (err != ESP_OK) {
        return blusys_translate_esp_err(err);
    }

    if (!verify_requested) {
        return BLUSYS_OK;
    }

    const blusys_err_t vrc = verify_partition_sha256(staged, (size_t)image_size, expected);
    if (vrc == BLUSYS_OK) {
        ESP_LOGI(BLUSYS_OTA_TAG, "sha256 verified (%d bytes)", image_size);
        return BLUSYS_OK;
    }

    /* Checksum mismatch (or read error). esp_https_ota_finish has already
       set the new partition as the next boot target — revert to the
       currently-running partition so the device does not boot a tampered
       or corrupt image. */
    ESP_LOGE(BLUSYS_OTA_TAG, "sha256 verify failed (rc=%d) — reverting boot partition", (int)vrc);
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running != NULL) {
        esp_err_t rerr = esp_ota_set_boot_partition(running);
        if (rerr != ESP_OK) {
            ESP_LOGE(BLUSYS_OTA_TAG, "revert set_boot_partition failed: 0x%x", rerr);
        }
    }
    return vrc;
}

blusys_err_t blusys_ota_close(blusys_ota_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    free(handle);
    return BLUSYS_OK;
}

void blusys_ota_restart(void)
{
    blusys_system_restart();
}

blusys_err_t blusys_ota_mark_valid(void)
{
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    return blusys_translate_esp_err(err);
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_ota_open(const blusys_ota_config_t *config, blusys_ota_t **out_handle)
{
    (void) config;
    (void) out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ota_perform(blusys_ota_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ota_close(blusys_ota_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

void blusys_ota_restart(void)
{
}

blusys_err_t blusys_ota_mark_valid(void)
{
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_WIFI_SUPPORTED */
