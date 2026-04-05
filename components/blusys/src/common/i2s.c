#include "blusys/i2s.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_I2S_SUPPORTED

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"

struct blusys_i2s_tx {
    int port;
    int bclk_pin;
    int ws_pin;
    int dout_pin;
    int mclk_pin;
    uint32_t sample_rate_hz;
    bool started;
    bool closing;
    i2s_chan_handle_t channel_handle;
    blusys_lock_t lock;
};

static bool blusys_i2s_is_valid_port(int port)
{
    return (port >= 0) && ((unsigned int) port < SOC_I2S_NUM);
}

static bool blusys_i2s_is_valid_output_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_i2s_is_valid_optional_output_pin(int pin)
{
    return (pin < 0) || GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_i2s_is_valid_sample_rate(uint32_t sample_rate_hz)
{
    return sample_rate_hz > 0u;
}

static bool blusys_i2s_timeout_is_valid(int timeout_ms)
{
    return blusys_timeout_ms_is_valid(timeout_ms);
}

static blusys_err_t blusys_i2s_tx_config_is_valid(const blusys_i2s_tx_config_t *config)
{
    if (config == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!blusys_i2s_is_valid_port(config->port) ||
        !blusys_i2s_is_valid_output_pin(config->bclk_pin) ||
        !blusys_i2s_is_valid_output_pin(config->ws_pin) ||
        !blusys_i2s_is_valid_output_pin(config->dout_pin) ||
        !blusys_i2s_is_valid_optional_output_pin(config->mclk_pin) ||
        !blusys_i2s_is_valid_sample_rate(config->sample_rate_hz)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return BLUSYS_OK;
}

static blusys_err_t blusys_i2s_tx_set_started(blusys_i2s_tx_t *i2s, bool started)
{
    i2s->started = started;
    return BLUSYS_OK;
}

blusys_err_t blusys_i2s_tx_open(const blusys_i2s_tx_config_t *config,
                                blusys_i2s_tx_t **out_i2s)
{
    blusys_i2s_tx_t *i2s;
    blusys_err_t err;
    i2s_chan_config_t channel_config;
    i2s_std_config_t std_config;
    esp_err_t esp_err;

    if (out_i2s == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_i2s_tx_config_is_valid(config);
    if (err != BLUSYS_OK) {
        return err;
    }

    i2s = calloc(1, sizeof(*i2s));
    if (i2s == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&i2s->lock);
    if (err != BLUSYS_OK) {
        free(i2s);
        return err;
    }

    i2s->port = config->port;
    i2s->bclk_pin = config->bclk_pin;
    i2s->ws_pin = config->ws_pin;
    i2s->dout_pin = config->dout_pin;
    i2s->mclk_pin = config->mclk_pin;
    i2s->sample_rate_hz = config->sample_rate_hz;

    channel_config = (i2s_chan_config_t) I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t) config->port,
                                                                     I2S_ROLE_MASTER);
    esp_err = i2s_new_channel(&channel_config, &i2s->channel_handle, NULL);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    std_config = (i2s_std_config_t) {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate_hz),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = (config->mclk_pin < 0) ? I2S_GPIO_UNUSED : (gpio_num_t) config->mclk_pin,
            .bclk = (gpio_num_t) config->bclk_pin,
            .ws = (gpio_num_t) config->ws_pin,
            .dout = (gpio_num_t) config->dout_pin,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = 0u,
                .bclk_inv = 0u,
                .ws_inv = 0u,
            },
        },
    };
    esp_err = i2s_channel_init_std_mode(i2s->channel_handle, &std_config);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_channel;
    }

    *out_i2s = i2s;
    return BLUSYS_OK;

fail_channel:
    i2s_del_channel(i2s->channel_handle);
fail_lock:
    blusys_lock_deinit(&i2s->lock);
    free(i2s);
    return err;
}

blusys_err_t blusys_i2s_tx_close(blusys_i2s_tx_t *i2s)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (i2s == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2s->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    i2s->closing = true;

    if (i2s->started) {
        esp_err = i2s_channel_disable(i2s->channel_handle);
        err = blusys_translate_esp_err(esp_err);
        if ((err != BLUSYS_OK) && (err != BLUSYS_ERR_INVALID_STATE)) {
            i2s->closing = false;
            blusys_lock_give(&i2s->lock);
            return err;
        }
        blusys_i2s_tx_set_started(i2s, false);
    }

    err = blusys_translate_esp_err(i2s_del_channel(i2s->channel_handle));
    if (err != BLUSYS_OK) {
        i2s->closing = false;
        blusys_lock_give(&i2s->lock);
        return err;
    }

    blusys_lock_give(&i2s->lock);
    blusys_lock_deinit(&i2s->lock);
    free(i2s);
    return BLUSYS_OK;
}

blusys_err_t blusys_i2s_tx_start(blusys_i2s_tx_t *i2s)
{
    blusys_err_t err;

    if (i2s == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2s->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (i2s->closing) {
        blusys_lock_give(&i2s->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(i2s_channel_enable(i2s->channel_handle));
    if (err == BLUSYS_OK) {
        blusys_i2s_tx_set_started(i2s, true);
    }

    blusys_lock_give(&i2s->lock);
    return err;
}

blusys_err_t blusys_i2s_tx_stop(blusys_i2s_tx_t *i2s)
{
    blusys_err_t err;

    if (i2s == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2s->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (i2s->closing) {
        blusys_lock_give(&i2s->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(i2s_channel_disable(i2s->channel_handle));
    if (err == BLUSYS_OK) {
        blusys_i2s_tx_set_started(i2s, false);
    }

    blusys_lock_give(&i2s->lock);
    return err;
}

blusys_err_t blusys_i2s_tx_write(blusys_i2s_tx_t *i2s,
                                 const void *data,
                                 size_t size,
                                 int timeout_ms)
{
    blusys_err_t err;
    size_t bytes_written = 0u;

    if ((i2s == NULL) || (data == NULL) || (size == 0u) || !blusys_i2s_timeout_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&i2s->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (i2s->closing || !i2s->started) {
        blusys_lock_give(&i2s->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(i2s_channel_write(i2s->channel_handle,
                                                     data,
                                                     size,
                                                     &bytes_written,
                                                     (uint32_t) timeout_ms));
    if ((err == BLUSYS_OK) && (bytes_written != size)) {
        err = BLUSYS_ERR_IO;
    }

    blusys_lock_give(&i2s->lock);
    return err;
}

#else

blusys_err_t blusys_i2s_tx_open(const blusys_i2s_tx_config_t *config,
                                blusys_i2s_tx_t **out_i2s)
{
    (void) config;
    (void) out_i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_tx_close(blusys_i2s_tx_t *i2s)
{
    (void) i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_tx_start(blusys_i2s_tx_t *i2s)
{
    (void) i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_tx_stop(blusys_i2s_tx_t *i2s)
{
    (void) i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_tx_write(blusys_i2s_tx_t *i2s,
                                 const void *data,
                                 size_t size,
                                 int timeout_ms)
{
    (void) i2s;
    (void) data;
    (void) size;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
