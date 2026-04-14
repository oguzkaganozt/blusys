#include "blusys/hal/i2s.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_I2S_SUPPORTED

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"

struct blusys_i2s_rx {
    int port;
    bool started;
    bool closing;
    i2s_chan_handle_t channel_handle;
    blusys_lock_t lock;
};

static bool blusys_i2s_rx_config_is_valid(const blusys_i2s_rx_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if ((config->port < 0) || ((unsigned int) config->port >= SOC_I2S_NUM)) {
        return false;
    }

    if (!GPIO_IS_VALID_OUTPUT_GPIO(config->bclk_pin) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->ws_pin) ||
        !GPIO_IS_VALID_GPIO(config->din_pin)) {
        return false;
    }

    if ((config->mclk_pin >= 0) && !GPIO_IS_VALID_OUTPUT_GPIO(config->mclk_pin)) {
        return false;
    }

    return config->sample_rate_hz > 0u;
}

blusys_err_t blusys_i2s_rx_open(const blusys_i2s_rx_config_t *config,
                                blusys_i2s_rx_t **out_i2s)
{
    blusys_i2s_rx_t *i2s;
    blusys_err_t err;
    i2s_chan_config_t channel_config;
    i2s_std_config_t std_config;
    esp_err_t esp_err;

    if (out_i2s == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!blusys_i2s_rx_config_is_valid(config)) {
        return BLUSYS_ERR_INVALID_ARG;
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

    channel_config = (i2s_chan_config_t) I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t) config->port,
                                                                     I2S_ROLE_MASTER);
    esp_err = i2s_new_channel(&channel_config, NULL, &i2s->channel_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    std_config = (i2s_std_config_t) {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate_hz),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = (config->mclk_pin < 0) ? I2S_GPIO_UNUSED : (gpio_num_t) config->mclk_pin,
            .bclk = (gpio_num_t) config->bclk_pin,
            .ws   = (gpio_num_t) config->ws_pin,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t) config->din_pin,
            .invert_flags = {
                .mclk_inv = 0u,
                .bclk_inv = 0u,
                .ws_inv   = 0u,
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

blusys_err_t blusys_i2s_rx_close(blusys_i2s_rx_t *i2s)
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
        i2s->started = false;
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

blusys_err_t blusys_i2s_rx_start(blusys_i2s_rx_t *i2s)
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
        i2s->started = true;
    }

    blusys_lock_give(&i2s->lock);
    return err;
}

blusys_err_t blusys_i2s_rx_stop(blusys_i2s_rx_t *i2s)
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
        i2s->started = false;
    }

    blusys_lock_give(&i2s->lock);
    return err;
}

blusys_err_t blusys_i2s_rx_read(blusys_i2s_rx_t *i2s,
                                void *buf,
                                size_t size,
                                size_t *bytes_read,
                                int timeout_ms)
{
    blusys_err_t err;
    size_t read = 0u;

    if ((i2s == NULL) || (buf == NULL) || (size == 0u) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
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

    err = blusys_translate_esp_err(
        i2s_channel_read(i2s->channel_handle, buf, size, &read, (uint32_t) timeout_ms));

    if (bytes_read != NULL) {
        *bytes_read = read;
    }

    blusys_lock_give(&i2s->lock);
    return err;
}

#else

blusys_err_t blusys_i2s_rx_open(const blusys_i2s_rx_config_t *config,
                                blusys_i2s_rx_t **out_i2s)
{
    (void) config;
    (void) out_i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_rx_close(blusys_i2s_rx_t *i2s)
{
    (void) i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_rx_start(blusys_i2s_rx_t *i2s)
{
    (void) i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_rx_stop(blusys_i2s_rx_t *i2s)
{
    (void) i2s;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2s_rx_read(blusys_i2s_rx_t *i2s,
                                void *buf,
                                size_t size,
                                size_t *bytes_read,
                                int timeout_ms)
{
    (void) i2s;
    (void) buf;
    (void) size;
    (void) bytes_read;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
