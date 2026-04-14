#include "blusys/drivers/led_strip.h"

#include "soc/soc_caps.h"

#if SOC_RMT_SUPPORTED

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "hal/rmt_types.h"

#define LED_STRIP_RMT_RESOLUTION_HZ     10000000u   /* 10 MHz = 100 ns/tick */
#define LED_STRIP_RMT_MEM_BLOCK_SYMBOLS 64u
#define LED_STRIP_RMT_TRANS_QUEUE_DEPTH 1u

/* Per-chip timing and color-order parameters.
 * All durations are in ticks at 10 MHz (1 tick = 100 ns). */
typedef struct {
    uint16_t bit0_high;
    uint16_t bit0_low;
    uint16_t bit1_high;
    uint16_t bit1_low;
    uint16_t reset;
    uint8_t  bytes_per_pixel;   /* 3 = RGB/GRB, 4 = GRBW */
    uint8_t  r_offset;
    uint8_t  g_offset;
    uint8_t  b_offset;
    uint8_t  w_offset;          /* only meaningful when bytes_per_pixel == 4 */
} led_strip_params_t;

static const led_strip_params_t s_params[] = {
    /* WS2812B: GRB, 400/850 ns high, 800/450 ns low */
    [BLUSYS_LED_STRIP_WS2812B] = {
        .bit0_high = 4, .bit0_low = 9,
        .bit1_high = 8, .bit1_low = 5,
        .reset = 600,
        .bytes_per_pixel = 3,
        .r_offset = 1, .g_offset = 0, .b_offset = 2, .w_offset = 0,
    },
    /* SK6812 RGB: GRB, 300/900 ns high, 600/600 ns low */
    [BLUSYS_LED_STRIP_SK6812_RGB] = {
        .bit0_high = 3, .bit0_low = 9,
        .bit1_high = 6, .bit1_low = 6,
        .reset = 800,
        .bytes_per_pixel = 3,
        .r_offset = 1, .g_offset = 0, .b_offset = 2, .w_offset = 0,
    },
    /* SK6812 RGBW: GRBW, same timing as SK6812 RGB */
    [BLUSYS_LED_STRIP_SK6812_RGBW] = {
        .bit0_high = 3, .bit0_low = 9,
        .bit1_high = 6, .bit1_low = 6,
        .reset = 800,
        .bytes_per_pixel = 4,
        .r_offset = 1, .g_offset = 0, .b_offset = 2, .w_offset = 3,
    },
    /* WS2811: RGB, 500/2000 ns high, 1200/1300 ns low */
    [BLUSYS_LED_STRIP_WS2811] = {
        .bit0_high = 5, .bit0_low = 20,
        .bit1_high = 12, .bit1_low = 13,
        .reset = 600,
        .bytes_per_pixel = 3,
        .r_offset = 0, .g_offset = 1, .b_offset = 2, .w_offset = 0,
    },
    /* APA106: RGB, 350/1360 ns high, 1360/350 ns low */
    [BLUSYS_LED_STRIP_APA106] = {
        .bit0_high = 4, .bit0_low = 14,
        .bit1_high = 14, .bit1_low = 4,
        .reset = 600,
        .bytes_per_pixel = 3,
        .r_offset = 0, .g_offset = 1, .b_offset = 2, .w_offset = 0,
    },
};

#define LED_STRIP_TYPE_COUNT (sizeof(s_params) / sizeof(s_params[0]))

struct blusys_led_strip {
    rmt_channel_handle_t    rmt_chan;
    rmt_encoder_handle_t    bytes_enc;
    rmt_encoder_handle_t    reset_enc;
    blusys_lock_t           lock;
    const led_strip_params_t *params;
    uint32_t                led_count;
    uint8_t                *buf;
};

blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip)
{
    blusys_led_strip_t *strip;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((config == NULL) || (out_strip == NULL) ||
        (config->led_count == 0u) || !GPIO_IS_VALID_OUTPUT_GPIO(config->pin) ||
        ((uint32_t) config->type >= LED_STRIP_TYPE_COUNT)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    const led_strip_params_t *params = &s_params[config->type];

    strip = calloc(1, sizeof(*strip));
    if (strip == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    strip->led_count = config->led_count;
    strip->params    = params;

    strip->buf = calloc((size_t) config->led_count * params->bytes_per_pixel, 1u);
    if (strip->buf == NULL) {
        free(strip);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&strip->lock);
    if (err != BLUSYS_OK) {
        goto fail_buf;
    }

    esp_err = rmt_new_tx_channel(&(rmt_tx_channel_config_t) {
        .gpio_num          = config->pin,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = LED_STRIP_RMT_RESOLUTION_HZ,
        .mem_block_symbols = LED_STRIP_RMT_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = LED_STRIP_RMT_TRANS_QUEUE_DEPTH,
        .intr_priority     = 0,
        .flags.invert_out  = 0u,
        .flags.with_dma    = 0u,
        .flags.io_loop_back = 0u,
        .flags.io_od_mode  = 0u,
        .flags.allow_pd    = 0u,
        .flags.init_level  = 0u,
    }, &strip->rmt_chan);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = rmt_new_bytes_encoder(&(rmt_bytes_encoder_config_t) {
        .bit0 = {
            .level0    = 1, .duration0 = params->bit0_high,
            .level1    = 0, .duration1 = params->bit0_low,
        },
        .bit1 = {
            .level0    = 1, .duration0 = params->bit1_high,
            .level1    = 0, .duration1 = params->bit1_low,
        },
        .flags.msb_first = 1u,
    }, &strip->bytes_enc);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_channel;
    }

    esp_err = rmt_new_copy_encoder(&(rmt_copy_encoder_config_t) {}, &strip->reset_enc);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_bytes_enc;
    }

    esp_err = rmt_enable(strip->rmt_chan);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_copy_enc;
    }

    *out_strip = strip;
    return BLUSYS_OK;

fail_copy_enc:
    rmt_del_encoder(strip->reset_enc);
fail_bytes_enc:
    rmt_del_encoder(strip->bytes_enc);
fail_channel:
    rmt_del_channel(strip->rmt_chan);
fail_lock:
    blusys_lock_deinit(&strip->lock);
fail_buf:
    free(strip->buf);
    free(strip);
    return err;
}

blusys_err_t blusys_led_strip_close(blusys_led_strip_t *strip)
{
    blusys_err_t err;

    if (strip == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&strip->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    rmt_disable(strip->rmt_chan);
    rmt_del_encoder(strip->reset_enc);
    rmt_del_encoder(strip->bytes_enc);
    rmt_del_channel(strip->rmt_chan);

    blusys_lock_give(&strip->lock);
    blusys_lock_deinit(&strip->lock);

    free(strip->buf);
    free(strip);
    return BLUSYS_OK;
}

blusys_err_t blusys_led_strip_set_pixel(blusys_led_strip_t *strip,
                                         uint32_t index,
                                         uint8_t r, uint8_t g, uint8_t b)
{
    if (strip == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (index >= strip->led_count) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    const led_strip_params_t *p = strip->params;
    uint8_t *pixel = &strip->buf[index * p->bytes_per_pixel];

    pixel[p->r_offset] = r;
    pixel[p->g_offset] = g;
    pixel[p->b_offset] = b;
    if (p->bytes_per_pixel == 4u) {
        pixel[p->w_offset] = 0u;   /* clear W so stale data is never sent */
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_led_strip_set_pixel_rgbw(blusys_led_strip_t *strip,
                                              uint32_t index,
                                              uint8_t r, uint8_t g, uint8_t b,
                                              uint8_t w)
{
    if (strip == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (index >= strip->led_count) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    const led_strip_params_t *p = strip->params;

    if (p->bytes_per_pixel != 4u) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }

    uint8_t *pixel = &strip->buf[index * p->bytes_per_pixel];
    pixel[p->r_offset] = r;
    pixel[p->g_offset] = g;
    pixel[p->b_offset] = b;
    pixel[p->w_offset] = w;

    return BLUSYS_OK;
}

blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if ((strip == NULL) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    size_t buf_size = (size_t) strip->led_count * strip->params->bytes_per_pixel;

    /* Build the reset symbol dynamically from the chip's reset duration */
    rmt_symbol_word_t reset_sym = {
        .level0    = 0,
        .duration0 = strip->params->reset,
        .level1    = 0,
        .duration1 = 0,
    };

    err = blusys_lock_take(&strip->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = rmt_transmit(strip->rmt_chan,
                            strip->bytes_enc,
                            strip->buf,
                            buf_size,
                            &(rmt_transmit_config_t) {
                                .loop_count              = 0,
                                .flags.eot_level         = 0u,
                                .flags.queue_nonblocking = 0u,
                            });
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto done;
    }

    esp_err = rmt_tx_wait_all_done(strip->rmt_chan, blusys_timeout_ms_to_ticks(timeout_ms));
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto done;
    }

    esp_err = rmt_transmit(strip->rmt_chan,
                            strip->reset_enc,
                            &reset_sym,
                            sizeof(reset_sym),
                            &(rmt_transmit_config_t) {
                                .loop_count              = 0,
                                .flags.eot_level         = 0u,
                                .flags.queue_nonblocking = 0u,
                            });
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto done;
    }

    err = blusys_translate_esp_err(rmt_tx_wait_all_done(strip->rmt_chan,
                                                         blusys_timeout_ms_to_ticks(timeout_ms)));

done:
    blusys_lock_give(&strip->lock);
    return err;
}

blusys_err_t blusys_led_strip_clear(blusys_led_strip_t *strip, int timeout_ms)
{
    if (strip == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    memset(strip->buf, 0, (size_t) strip->led_count * strip->params->bytes_per_pixel);
    return blusys_led_strip_refresh(strip, timeout_ms);
}

#else /* !SOC_RMT_SUPPORTED */

blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip)
{
    (void) config;
    (void) out_strip;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_led_strip_close(blusys_led_strip_t *strip)
{
    (void) strip;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_led_strip_set_pixel(blusys_led_strip_t *strip,
                                         uint32_t index,
                                         uint8_t r, uint8_t g, uint8_t b)
{
    (void) strip;
    (void) index;
    (void) r;
    (void) g;
    (void) b;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_led_strip_set_pixel_rgbw(blusys_led_strip_t *strip,
                                              uint32_t index,
                                              uint8_t r, uint8_t g, uint8_t b,
                                              uint8_t w)
{
    (void) strip;
    (void) index;
    (void) r;
    (void) g;
    (void) b;
    (void) w;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms)
{
    (void) strip;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_led_strip_clear(blusys_led_strip_t *strip, int timeout_ms)
{
    (void) strip;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_RMT_SUPPORTED */
