#include "blusys/adc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"

#define BLUSYS_ADC_CHANNEL_MASK(channel) (1u << (unsigned int) (channel))

struct blusys_adc {
    int pin;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_oneshot_unit_handle_t unit_handle;
    adc_cali_handle_t cali_handle;
    bool cali_ready;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_adc_state_lock = portMUX_INITIALIZER_UNLOCKED;
static adc_oneshot_unit_handle_t blusys_adc_unit1_handle;
static unsigned int blusys_adc_unit1_ref_count;
static uint32_t blusys_adc_unit1_channel_mask;

static bool blusys_adc_is_valid_atten(blusys_adc_atten_t atten)
{
    return (atten >= BLUSYS_ADC_ATTEN_DB_0) && (atten <= BLUSYS_ADC_ATTEN_DB_12);
}

static adc_atten_t blusys_adc_to_esp_atten(blusys_adc_atten_t atten)
{
    switch (atten) {
    case BLUSYS_ADC_ATTEN_DB_0:
        return ADC_ATTEN_DB_0;
    case BLUSYS_ADC_ATTEN_DB_2_5:
        return ADC_ATTEN_DB_2_5;
    case BLUSYS_ADC_ATTEN_DB_6:
        return ADC_ATTEN_DB_6;
    case BLUSYS_ADC_ATTEN_DB_12:
    default:
        return ADC_ATTEN_DB_12;
    }
}

static bool blusys_adc_supports_calibration(adc_atten_t atten)
{
    return (atten == ADC_ATTEN_DB_0) || (atten == ADC_ATTEN_DB_12);
}

static blusys_err_t blusys_adc_resolve_pin(int pin, adc_channel_t *out_channel)
{
    adc_unit_t unit;
    esp_err_t esp_err;

    if (out_channel == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err = adc_oneshot_io_to_channel(pin, &unit, out_channel);
    if (esp_err == ESP_ERR_NOT_FOUND) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }
    if (unit != ADC_UNIT_1) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return BLUSYS_OK;
}

static blusys_err_t blusys_adc_acquire_unit(adc_channel_t channel, adc_oneshot_unit_handle_t *out_handle)
{
    adc_oneshot_unit_handle_t new_handle = NULL;
    uint32_t channel_mask = BLUSYS_ADC_CHANNEL_MASK(channel);
    esp_err_t esp_err = ESP_OK;

    if (out_handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&blusys_adc_state_lock);
    if ((blusys_adc_unit1_channel_mask & channel_mask) != 0u) {
        portEXIT_CRITICAL(&blusys_adc_state_lock);
        return BLUSYS_ERR_BUSY;
    }
    blusys_adc_unit1_channel_mask |= channel_mask;
    if (blusys_adc_unit1_handle != NULL) {
        blusys_adc_unit1_ref_count += 1u;
        *out_handle = blusys_adc_unit1_handle;
        portEXIT_CRITICAL(&blusys_adc_state_lock);
        return BLUSYS_OK;
    }
    portEXIT_CRITICAL(&blusys_adc_state_lock);

    esp_err = adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t) {
        .unit_id = ADC_UNIT_1,
        .clk_src = 0,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    }, &new_handle);
    if (esp_err != ESP_OK) {
        portENTER_CRITICAL(&blusys_adc_state_lock);
        blusys_adc_unit1_channel_mask &= ~channel_mask;
        portEXIT_CRITICAL(&blusys_adc_state_lock);
        return blusys_translate_esp_err(esp_err);
    }

    portENTER_CRITICAL(&blusys_adc_state_lock);
    if (blusys_adc_unit1_handle == NULL) {
        blusys_adc_unit1_handle = new_handle;
        blusys_adc_unit1_ref_count = 1u;
        *out_handle = blusys_adc_unit1_handle;
        portEXIT_CRITICAL(&blusys_adc_state_lock);
        return BLUSYS_OK;
    }

    blusys_adc_unit1_ref_count += 1u;
    *out_handle = blusys_adc_unit1_handle;
    portEXIT_CRITICAL(&blusys_adc_state_lock);

    adc_oneshot_del_unit(new_handle);
    return BLUSYS_OK;
}

static void blusys_adc_release_unit(adc_channel_t channel)
{
    adc_oneshot_unit_handle_t handle_to_delete = NULL;
    uint32_t channel_mask = BLUSYS_ADC_CHANNEL_MASK(channel);

    portENTER_CRITICAL(&blusys_adc_state_lock);
    blusys_adc_unit1_channel_mask &= ~channel_mask;
    if (blusys_adc_unit1_ref_count > 0u) {
        blusys_adc_unit1_ref_count -= 1u;
        if (blusys_adc_unit1_ref_count == 0u) {
            handle_to_delete = blusys_adc_unit1_handle;
            blusys_adc_unit1_handle = NULL;
        }
    }
    portEXIT_CRITICAL(&blusys_adc_state_lock);

    if (handle_to_delete != NULL) {
        adc_oneshot_del_unit(handle_to_delete);
    }
}

static blusys_err_t blusys_adc_try_init_calibration(blusys_adc_t *adc)
{
    if (!blusys_adc_supports_calibration(adc->atten)) {
        adc->cali_handle = NULL;
        adc->cali_ready = false;
        return BLUSYS_OK;
    }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    esp_err_t esp_err;

    esp_err = adc_cali_create_scheme_curve_fitting(&(adc_cali_curve_fitting_config_t) {
        .unit_id = ADC_UNIT_1,
        .chan = adc->channel,
        .atten = adc->atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    }, &adc->cali_handle);
    if (esp_err == ESP_OK) {
        adc->cali_ready = true;
        return BLUSYS_OK;
    }
    if (esp_err == ESP_ERR_NOT_SUPPORTED) {
        adc->cali_handle = NULL;
        adc->cali_ready = false;
        return BLUSYS_OK;
    }

    return blusys_translate_esp_err(esp_err);
#else
    (void) adc;
    return BLUSYS_OK;
#endif
}

static void blusys_adc_deinit_calibration(blusys_adc_t *adc)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (adc->cali_ready) {
        adc_cali_delete_scheme_curve_fitting(adc->cali_handle);
        adc->cali_handle = NULL;
        adc->cali_ready = false;
    }
#else
    (void) adc;
#endif
}

blusys_err_t blusys_adc_open(int pin, blusys_adc_atten_t atten, blusys_adc_t **out_adc)
{
    blusys_adc_t *adc;
    blusys_err_t err;

    if ((out_adc == NULL) || !blusys_adc_is_valid_atten(atten)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    adc = calloc(1, sizeof(*adc));
    if (adc == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&adc->lock);
    if (err != BLUSYS_OK) {
        free(adc);
        return err;
    }

    adc->pin = pin;
    adc->atten = blusys_adc_to_esp_atten(atten);

    err = blusys_adc_resolve_pin(pin, &adc->channel);
    if (err != BLUSYS_OK) {
        goto fail_lock;
    }

    err = blusys_adc_acquire_unit(adc->channel, &adc->unit_handle);
    if (err != BLUSYS_OK) {
        goto fail_lock;
    }

    err = blusys_translate_esp_err(adc_oneshot_config_channel(adc->unit_handle,
                                                              adc->channel,
                                                              &(adc_oneshot_chan_cfg_t) {
                                                                  .atten = adc->atten,
                                                                  .bitwidth = ADC_BITWIDTH_DEFAULT,
                                                              }));
    if (err != BLUSYS_OK) {
        goto fail_unit;
    }

    err = blusys_adc_try_init_calibration(adc);
    if (err != BLUSYS_OK) {
        goto fail_unit;
    }

    *out_adc = adc;
    return BLUSYS_OK;

fail_unit:
    blusys_adc_release_unit(adc->channel);
fail_lock:
    blusys_lock_deinit(&adc->lock);
    free(adc);
    return err;
}

blusys_err_t blusys_adc_close(blusys_adc_t *adc)
{
    blusys_err_t err;

    if (adc == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&adc->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_adc_deinit_calibration(adc);

    blusys_lock_give(&adc->lock);
    blusys_lock_deinit(&adc->lock);
    blusys_adc_release_unit(adc->channel);
    free(adc);

    return BLUSYS_OK;
}

blusys_err_t blusys_adc_read_raw(blusys_adc_t *adc, int *out_raw)
{
    blusys_err_t err;

    if ((adc == NULL) || (out_raw == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&adc->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(adc_oneshot_read(adc->unit_handle, adc->channel, out_raw));

    blusys_lock_give(&adc->lock);

    return err;
}

blusys_err_t blusys_adc_read_mv(blusys_adc_t *adc, int *out_mv)
{
    blusys_err_t err;

    if ((adc == NULL) || (out_mv == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (!adc->cali_ready) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }

    err = blusys_lock_take(&adc->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(adc_oneshot_get_calibrated_result(adc->unit_handle,
                                                                     adc->cali_handle,
                                                                     adc->channel,
                                                                     out_mv));

    blusys_lock_give(&adc->lock);

    return err;
}
