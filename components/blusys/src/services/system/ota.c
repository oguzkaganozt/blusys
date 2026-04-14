#include "blusys/services/system/ota.h"

#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"

#include "blusys/hal/system.h"
#include "blusys/hal/internal/esp_err_shim.h"

#define DEFAULT_TIMEOUT_MS 30000

struct blusys_ota {
    const char *url;
    const char *cert_pem;
    int         timeout_ms;
    blusys_ota_progress_cb_t on_progress;
    void       *progress_ctx;
};

blusys_err_t blusys_ota_open(const blusys_ota_config_t *config, blusys_ota_t **out_handle)
{
    if ((config == NULL) || (out_handle == NULL) || (config->url == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_ota_t *h = malloc(sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    h->url           = config->url;
    h->cert_pem      = config->cert_pem;
    h->timeout_ms    = config->timeout_ms;
    h->on_progress   = config->on_progress;
    h->progress_ctx  = config->progress_ctx;

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

    err = esp_https_ota_finish(esp_ota_handle);
    return blusys_translate_esp_err(err);
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
