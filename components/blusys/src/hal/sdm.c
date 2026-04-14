#include "blusys/hal/sdm.h"

#include "soc/soc_caps.h"

#if SOC_SDM_SUPPORTED

#include <stddef.h>
#include <stdlib.h>

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#include "driver/gpio.h"
#include "driver/sdm.h"

struct blusys_sdm {
    sdm_channel_handle_t channel;
    blusys_lock_t        lock;
};

blusys_err_t blusys_sdm_open(int pin, uint32_t sample_rate_hz, blusys_sdm_t **out_sdm)
{
    blusys_sdm_t *h;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin) ||
        (sample_rate_hz == 0u) ||
        (out_sdm == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    sdm_config_t cfg = {
        .gpio_num      = pin,
        .clk_src       = SDM_CLK_SRC_DEFAULT,
        .sample_rate_hz = sample_rate_hz,
    };
    esp_err = sdm_new_channel(&cfg, &h->channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = sdm_channel_enable(h->channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_channel;
    }

    esp_err = sdm_channel_set_pulse_density(h->channel, 0);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        sdm_channel_disable(h->channel);
        goto fail_channel;
    }

    *out_sdm = h;
    return BLUSYS_OK;

fail_channel:
    sdm_del_channel(h->channel);
fail_lock:
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_sdm_close(blusys_sdm_t *sdm)
{
    blusys_err_t err;

    if (sdm == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&sdm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    sdm_channel_disable(sdm->channel);
    sdm_del_channel(sdm->channel);

    blusys_lock_give(&sdm->lock);
    blusys_lock_deinit(&sdm->lock);
    free(sdm);

    return BLUSYS_OK;
}

blusys_err_t blusys_sdm_set_density(blusys_sdm_t *sdm, int8_t density)
{
    blusys_err_t err;

    if (sdm == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&sdm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(sdm_channel_set_pulse_density(sdm->channel, density));

    blusys_lock_give(&sdm->lock);
    return err;
}

#else

blusys_err_t blusys_sdm_open(int pin, uint32_t sample_rate_hz, blusys_sdm_t **out_sdm)
{
    (void) pin;
    (void) sample_rate_hz;
    (void) out_sdm;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sdm_close(blusys_sdm_t *sdm)
{
    (void) sdm;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sdm_set_density(blusys_sdm_t *sdm, int8_t density)
{
    (void) sdm;
    (void) density;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
