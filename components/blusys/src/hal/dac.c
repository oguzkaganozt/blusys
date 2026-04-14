#include "blusys/hal/dac.h"

#include <stdbool.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_DAC_SUPPORTED

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#include "driver/dac_oneshot.h"
#include "freertos/FreeRTOS.h"

#define BLUSYS_DAC_CHANNEL_MASK(channel) (1u << (unsigned int) (channel))

struct blusys_dac {
    int pin;
    dac_channel_t channel;
    bool closing;
    dac_oneshot_handle_t handle;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_dac_state_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned int blusys_dac_channel_mask;

static blusys_err_t blusys_dac_resolve_pin(int pin, dac_channel_t *out_channel)
{
    if (out_channel == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

#if CONFIG_IDF_TARGET_ESP32
    switch (pin) {
    case 25:
        *out_channel = DAC_CHAN_0;
        return BLUSYS_OK;
    case 26:
        *out_channel = DAC_CHAN_1;
        return BLUSYS_OK;
    default:
        return BLUSYS_ERR_INVALID_ARG;
    }
#else
    (void) pin;
    return BLUSYS_ERR_NOT_SUPPORTED;
#endif
}

static blusys_err_t blusys_dac_acquire_channel(dac_channel_t channel)
{
    unsigned int channel_mask = BLUSYS_DAC_CHANNEL_MASK(channel);

    portENTER_CRITICAL(&blusys_dac_state_lock);
    if ((blusys_dac_channel_mask & channel_mask) != 0u) {
        portEXIT_CRITICAL(&blusys_dac_state_lock);
        return BLUSYS_ERR_BUSY;
    }
    blusys_dac_channel_mask |= channel_mask;
    portEXIT_CRITICAL(&blusys_dac_state_lock);

    return BLUSYS_OK;
}

static void blusys_dac_release_channel(dac_channel_t channel)
{
    portENTER_CRITICAL(&blusys_dac_state_lock);
    blusys_dac_channel_mask &= ~BLUSYS_DAC_CHANNEL_MASK(channel);
    portEXIT_CRITICAL(&blusys_dac_state_lock);
}

blusys_err_t blusys_dac_open(int pin, blusys_dac_t **out_dac)
{
    blusys_dac_t *dac;
    blusys_err_t err;
    dac_channel_t channel;

    if (out_dac == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_dac_resolve_pin(pin, &channel);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_dac_acquire_channel(channel);
    if (err != BLUSYS_OK) {
        return err;
    }

    dac = calloc(1, sizeof(*dac));
    if (dac == NULL) {
        blusys_dac_release_channel(channel);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&dac->lock);
    if (err != BLUSYS_OK) {
        free(dac);
        blusys_dac_release_channel(channel);
        return err;
    }

    dac->pin = pin;
    dac->channel = channel;

    err = blusys_translate_esp_err(dac_oneshot_new_channel(&(dac_oneshot_config_t) {
                                                               .chan_id = channel,
                                                           },
                                                           &dac->handle));
    if (err != BLUSYS_OK) {
        goto fail_lock;
    }

    *out_dac = dac;
    return BLUSYS_OK;

fail_lock:
    blusys_lock_deinit(&dac->lock);
    free(dac);
    blusys_dac_release_channel(channel);
    return err;
}

blusys_err_t blusys_dac_close(blusys_dac_t *dac)
{
    blusys_err_t err;

    if (dac == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dac->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    dac->closing = true;

    err = blusys_translate_esp_err(dac_oneshot_del_channel(dac->handle));
    if (err != BLUSYS_OK) {
        dac->closing = false;
        blusys_lock_give(&dac->lock);
        return err;
    }
    dac->handle = NULL;

    blusys_lock_give(&dac->lock);
    blusys_lock_deinit(&dac->lock);
    blusys_dac_release_channel(dac->channel);
    free(dac);
    return BLUSYS_OK;
}

blusys_err_t blusys_dac_write(blusys_dac_t *dac, uint8_t value)
{
    blusys_err_t err;

    if (dac == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dac->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (dac->closing) {
        blusys_lock_give(&dac->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(dac_oneshot_output_voltage(dac->handle, value));

    blusys_lock_give(&dac->lock);
    return err;
}

#else

blusys_err_t blusys_dac_open(int pin, blusys_dac_t **out_dac)
{
    (void) pin;
    (void) out_dac;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_dac_close(blusys_dac_t *dac)
{
    (void) dac;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_dac_write(blusys_dac_t *dac, uint8_t value)
{
    (void) dac;
    (void) value;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
