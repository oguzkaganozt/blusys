#include "blusys/led_strip.h"

#include "soc/soc_caps.h"

#if SOC_RMT_SUPPORTED

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#include "driver/gpio.h"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "hal/rmt_types.h"

/* WS2812B at 10 MHz RMT clock (100 ns per tick):
 *
 *  bit0: HIGH 400 ns (4 ticks) + LOW 900 ns (9 ticks) — within ±150 ns spec
 *  bit1: HIGH 800 ns (8 ticks) + LOW 500 ns (5 ticks) — within ±150 ns spec
 *  reset: LOW > 50 µs — use 600 ticks = 60 µs
 */
#define LED_STRIP_RMT_RESOLUTION_HZ     10000000u
#define LED_STRIP_RMT_MEM_BLOCK_SYMBOLS 64u
#define LED_STRIP_RMT_TRANS_QUEUE_DEPTH 1u

#define WS2812_BIT0_HIGH_TICKS  4u
#define WS2812_BIT0_LOW_TICKS   9u
#define WS2812_BIT1_HIGH_TICKS  8u
#define WS2812_BIT1_LOW_TICKS   5u
#define WS2812_RESET_TICKS      600u
#define WS2812_BYTES_PER_PIXEL  3u    /* GRB order */

struct blusys_led_strip {
    rmt_channel_handle_t rmt_chan;
    rmt_encoder_handle_t bytes_enc;
    rmt_encoder_handle_t reset_enc;
    blusys_lock_t        lock;
    uint32_t             led_count;
    uint8_t             *buf;        /* GRB layout: 3 * led_count bytes */
};

/* Reset symbol: 60 µs LOW, terminated by a zero-duration second half */
static const rmt_symbol_word_t s_led_reset_sym = {
    .level0    = 0,
    .duration0 = WS2812_RESET_TICKS,
    .level1    = 0,
    .duration1 = 0,
};

blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip)
{
    blusys_led_strip_t *strip;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((config == NULL) || (out_strip == NULL) ||
        (config->led_count == 0u) || !GPIO_IS_VALID_OUTPUT_GPIO(config->pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    strip = calloc(1, sizeof(*strip));
    if (strip == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    strip->led_count = config->led_count;

    strip->buf = calloc((size_t) config->led_count * WS2812_BYTES_PER_PIXEL, 1u);
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
            .level0    = 1, .duration0 = WS2812_BIT0_HIGH_TICKS,
            .level1    = 0, .duration1 = WS2812_BIT0_LOW_TICKS,
        },
        .bit1 = {
            .level0    = 1, .duration0 = WS2812_BIT1_HIGH_TICKS,
            .level1    = 0, .duration1 = WS2812_BIT1_LOW_TICKS,
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

    /* WS2812B on-wire order is GRB */
    strip->buf[index * WS2812_BYTES_PER_PIXEL + 0u] = g;
    strip->buf[index * WS2812_BYTES_PER_PIXEL + 1u] = r;
    strip->buf[index * WS2812_BYTES_PER_PIXEL + 2u] = b;

    return BLUSYS_OK;
}

blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms)
{
    blusys_err_t err;
    esp_err_t esp_err;
    size_t buf_size;

    if ((strip == NULL) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    buf_size = (size_t) strip->led_count * WS2812_BYTES_PER_PIXEL;

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
                            &s_led_reset_sym,
                            sizeof(s_led_reset_sym),
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

    memset(strip->buf, 0, (size_t) strip->led_count * WS2812_BYTES_PER_PIXEL);
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
