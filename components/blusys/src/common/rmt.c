#include "blusys/rmt.h"

#include "soc/soc_caps.h"

#if SOC_RMT_SUPPORTED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_timeout.h"

#include "driver/gpio.h"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "hal/rmt_types.h"

#define BLUSYS_RMT_RESOLUTION_HZ 1000000u
#define BLUSYS_RMT_MEM_BLOCK_SYMBOLS 64u
#define BLUSYS_RMT_TRANS_QUEUE_DEPTH 1u
#define BLUSYS_RMT_MAX_DURATION_US 32767u

struct blusys_rmt {
    int pin;
    bool idle_level;
    bool closing;
    rmt_channel_handle_t channel_handle;
    rmt_encoder_handle_t encoder_handle;
    blusys_lock_t lock;
};

static bool blusys_rmt_is_valid_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_rmt_timeout_is_valid(int timeout_ms)
{
    return blusys_timeout_ms_is_valid(timeout_ms);
}

static bool blusys_rmt_is_valid_pulse(const blusys_rmt_pulse_t *pulse)
{
    return (pulse != NULL) && (pulse->duration_us > 0u) && (pulse->duration_us <= BLUSYS_RMT_MAX_DURATION_US);
}

static blusys_err_t blusys_rmt_pack_symbols(const blusys_rmt_pulse_t *pulses,
                                            size_t pulse_count,
                                            rmt_symbol_word_t **out_symbols,
                                            size_t *out_symbol_count)
{
    size_t symbol_count;
    rmt_symbol_word_t *symbols;
    size_t pulse_index;
    size_t symbol_index;

    if ((pulses == NULL) || (out_symbols == NULL) || (out_symbol_count == NULL) || (pulse_count == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    symbol_count = (pulse_count + 1u) / 2u;
    if (symbol_count > (SIZE_MAX / sizeof(*symbols))) {
        return BLUSYS_ERR_NO_MEM;
    }

    symbols = calloc(symbol_count, sizeof(*symbols));
    if (symbols == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    for (pulse_index = 0u, symbol_index = 0u; pulse_index < pulse_count; pulse_index += 2u, ++symbol_index) {
        const blusys_rmt_pulse_t *first = &pulses[pulse_index];

        if (!blusys_rmt_is_valid_pulse(first)) {
            free(symbols);
            return BLUSYS_ERR_INVALID_ARG;
        }

        symbols[symbol_index].level0 = first->level ? 1u : 0u;
        symbols[symbol_index].duration0 = (uint16_t) first->duration_us;

        if ((pulse_index + 1u) < pulse_count) {
            const blusys_rmt_pulse_t *second = &pulses[pulse_index + 1u];

            if (!blusys_rmt_is_valid_pulse(second)) {
                free(symbols);
                return BLUSYS_ERR_INVALID_ARG;
            }

            symbols[symbol_index].level1 = second->level ? 1u : 0u;
            symbols[symbol_index].duration1 = (uint16_t) second->duration_us;
        }
    }

    *out_symbols = symbols;
    *out_symbol_count = symbol_count;
    return BLUSYS_OK;
}

static blusys_err_t blusys_rmt_reset_channel(blusys_rmt_t *rmt)
{
    blusys_err_t err;

    err = blusys_translate_esp_err(rmt_disable(rmt->channel_handle));
    if ((err != BLUSYS_OK) && (err != BLUSYS_ERR_INVALID_STATE)) {
        return err;
    }

    return blusys_translate_esp_err(rmt_enable(rmt->channel_handle));
}

blusys_err_t blusys_rmt_open(int pin, bool idle_level, blusys_rmt_t **out_rmt)
{
    blusys_rmt_t *rmt;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((out_rmt == NULL) || !blusys_rmt_is_valid_pin(pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    rmt = calloc(1, sizeof(*rmt));
    if (rmt == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&rmt->lock);
    if (err != BLUSYS_OK) {
        free(rmt);
        return err;
    }

    rmt->pin = pin;
    rmt->idle_level = idle_level;

    esp_err = rmt_new_tx_channel(&(rmt_tx_channel_config_t) {
        .gpio_num = pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = BLUSYS_RMT_RESOLUTION_HZ,
        .mem_block_symbols = BLUSYS_RMT_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = BLUSYS_RMT_TRANS_QUEUE_DEPTH,
        .intr_priority = 0,
        .flags.invert_out = 0u,
        .flags.with_dma = 0u,
        .flags.io_loop_back = 0u,
        .flags.io_od_mode = 0u,
        .flags.allow_pd = 0u,
        .flags.init_level = idle_level ? 1u : 0u,
    }, &rmt->channel_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = rmt_new_copy_encoder(&(rmt_copy_encoder_config_t) {}, &rmt->encoder_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_channel;
    }

    esp_err = rmt_enable(rmt->channel_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_encoder;
    }

    *out_rmt = rmt;
    return BLUSYS_OK;

fail_encoder:
    rmt_del_encoder(rmt->encoder_handle);
fail_channel:
    rmt_del_channel(rmt->channel_handle);
fail_lock:
    blusys_lock_deinit(&rmt->lock);
    free(rmt);
    return err;
}

blusys_err_t blusys_rmt_close(blusys_rmt_t *rmt)
{
    blusys_err_t err;

    if (rmt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&rmt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    rmt->closing = true;

    err = blusys_translate_esp_err(rmt_disable(rmt->channel_handle));
    if ((err != BLUSYS_OK) && (err != BLUSYS_ERR_INVALID_STATE)) {
        goto fail;
    }

    err = blusys_translate_esp_err(rmt_del_encoder(rmt->encoder_handle));
    if (err != BLUSYS_OK) {
        goto fail;
    }

    err = blusys_translate_esp_err(rmt_del_channel(rmt->channel_handle));
    if (err != BLUSYS_OK) {
        goto fail;
    }

    blusys_lock_give(&rmt->lock);
    blusys_lock_deinit(&rmt->lock);
    free(rmt);
    return BLUSYS_OK;

fail:
    rmt->closing = false;
    blusys_lock_give(&rmt->lock);
    return err;
}

blusys_err_t blusys_rmt_write(blusys_rmt_t *rmt,
                              const blusys_rmt_pulse_t *pulses,
                              size_t pulse_count,
                              int timeout_ms)
{
    blusys_err_t err;
    rmt_symbol_word_t *symbols = NULL;
    size_t symbol_count = 0u;

    if ((rmt == NULL) || !blusys_rmt_timeout_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_rmt_pack_symbols(pulses, pulse_count, &symbols, &symbol_count);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&rmt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        free(symbols);
        return err;
    }

    if (rmt->closing) {
        blusys_lock_give(&rmt->lock);
        free(symbols);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(rmt_transmit(rmt->channel_handle,
                                                rmt->encoder_handle,
                                                symbols,
                                                symbol_count * sizeof(*symbols),
                                                &(rmt_transmit_config_t) {
                                                    .loop_count = 0,
                                                    .flags.eot_level = rmt->idle_level ? 1u : 0u,
                                                    .flags.queue_nonblocking = 0u,
                                                }));
    if (err == BLUSYS_OK) {
        err = blusys_translate_esp_err(rmt_tx_wait_all_done(rmt->channel_handle, timeout_ms));
        if (err == BLUSYS_ERR_TIMEOUT) {
            blusys_err_t reset_err = blusys_rmt_reset_channel(rmt);

            if (reset_err != BLUSYS_OK) {
                err = reset_err;
            }
        }
    }

    blusys_lock_give(&rmt->lock);
    free(symbols);
    return err;
}

#else

blusys_err_t blusys_rmt_open(int pin, bool idle_level, blusys_rmt_t **out_rmt)
{
    (void) pin;
    (void) idle_level;
    (void) out_rmt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_rmt_close(blusys_rmt_t *rmt)
{
    (void) rmt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_rmt_write(blusys_rmt_t *rmt,
                              const blusys_rmt_pulse_t *pulses,
                              size_t pulse_count,
                              int timeout_ms)
{
    (void) rmt;
    (void) pulses;
    (void) pulse_count;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
