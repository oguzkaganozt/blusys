#include "blusys/hal/rmt.h"

#include "soc/soc_caps.h"

#if SOC_RMT_SUPPORTED

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/rmt_common.h"
#include "driver/rmt_rx.h"
#include "hal/rmt_types.h"

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BLUSYS_RMT_RX_RESOLUTION_HZ   1000000u   /* 1 MHz → 1 µs per tick */
#define BLUSYS_RMT_RX_MEM_BLOCK_SYMS  128u
#define BLUSYS_RMT_RX_MAX_PULSES      (BLUSYS_RMT_RX_MEM_BLOCK_SYMS * 2u)

struct blusys_rmt_rx {
    rmt_channel_handle_t channel;
    rmt_symbol_word_t   *symbol_buf;        /* heap buffer passed to rmt_receive() */
    size_t               pulse_count;       /* written by ISR callback, read after notify */
    TaskHandle_t         waiting_task;      /* task to notify on frame complete */
    uint32_t             range_min_ns;      /* stored from open config */
    uint32_t             range_max_ns;      /* stored from open config */
    blusys_lock_t        lock;
};

static IRAM_ATTR bool blusys_rmt_rx_done_cb(rmt_channel_handle_t channel,
                                             const rmt_rx_done_event_data_t *edata,
                                             void *user_ctx)
{
    blusys_rmt_rx_t *h = (blusys_rmt_rx_t *) user_ctx;
    BaseType_t higher_prio_woken = pdFALSE;

    (void) channel;

    h->pulse_count = edata->num_symbols * 2u;

    if (h->waiting_task != NULL) {
        vTaskNotifyGiveFromISR(h->waiting_task, &higher_prio_woken);
        h->waiting_task = NULL;
    }

    return higher_prio_woken == pdTRUE;
}

blusys_err_t blusys_rmt_rx_open(int pin,
                                 const blusys_rmt_rx_config_t *config,
                                 blusys_rmt_rx_t **out_rmt_rx)
{
    blusys_rmt_rx_t *h;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!GPIO_IS_VALID_GPIO(pin) || (config == NULL) ||
        (config->signal_range_min_ns == 0u) ||
        (config->signal_range_max_ns == 0u) ||
        (out_rmt_rx == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    h->symbol_buf = calloc(BLUSYS_RMT_RX_MEM_BLOCK_SYMS, sizeof(*h->symbol_buf));
    if (h->symbol_buf == NULL) {
        free(h);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h->symbol_buf);
        free(h);
        return err;
    }

    rmt_rx_channel_config_t rx_cfg = {
        .gpio_num          = (gpio_num_t) pin,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = BLUSYS_RMT_RX_RESOLUTION_HZ,
        .mem_block_symbols = BLUSYS_RMT_RX_MEM_BLOCK_SYMS,
        .intr_priority     = 0,
    };
    esp_err = rmt_new_rx_channel(&rx_cfg, &h->channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = blusys_rmt_rx_done_cb,
    };
    esp_err = rmt_rx_register_event_callbacks(h->channel, &cbs, h);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_channel;
    }

    esp_err = rmt_enable(h->channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_channel;
    }

    h->range_min_ns = config->signal_range_min_ns;
    h->range_max_ns = config->signal_range_max_ns;

    *out_rmt_rx = h;
    return BLUSYS_OK;

fail_channel:
    rmt_del_channel(h->channel);
fail_lock:
    blusys_lock_deinit(&h->lock);
    free(h->symbol_buf);
    free(h);
    return err;
}

blusys_err_t blusys_rmt_rx_close(blusys_rmt_rx_t *rmt_rx)
{
    blusys_err_t err;

    if (rmt_rx == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&rmt_rx->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = rmt_disable(rmt_rx->channel);
    if (esp_err != ESP_OK) {
        blusys_lock_give(&rmt_rx->lock);
        return blusys_translate_esp_err(esp_err);
    }

    rmt_del_channel(rmt_rx->channel);

    blusys_lock_give(&rmt_rx->lock);
    blusys_lock_deinit(&rmt_rx->lock);
    free(rmt_rx->symbol_buf);
    free(rmt_rx);

    return BLUSYS_OK;
}

blusys_err_t blusys_rmt_rx_read(blusys_rmt_rx_t *rmt_rx,
                                 blusys_rmt_pulse_t *pulses,
                                 size_t max_pulses,
                                 size_t *out_count,
                                 int timeout_ms)
{
    blusys_err_t err;
    esp_err_t esp_err;
    rmt_receive_config_t rx_cfg;
    uint32_t notify_val;
    size_t i;

    if ((rmt_rx == NULL) || (pulses == NULL) || (max_pulses == 0u) ||
        (out_count == NULL) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&rmt_rx->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    /* Reject concurrent receives — waiting_task serves as the busy indicator. */
    if (rmt_rx->waiting_task != NULL) {
        blusys_lock_give(&rmt_rx->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    rmt_rx->pulse_count  = 0u;
    rmt_rx->waiting_task = xTaskGetCurrentTaskHandle();

    rx_cfg.signal_range_min_ns = rmt_rx->range_min_ns;
    rx_cfg.signal_range_max_ns = rmt_rx->range_max_ns;

    esp_err = rmt_receive(rmt_rx->channel,
                          rmt_rx->symbol_buf,
                          BLUSYS_RMT_RX_MEM_BLOCK_SYMS * sizeof(*rmt_rx->symbol_buf),
                          &rx_cfg);
    if (esp_err != ESP_OK) {
        rmt_rx->waiting_task = NULL;
        blusys_lock_give(&rmt_rx->lock);
        return blusys_translate_esp_err(esp_err);
    }

    /* Release lock before blocking — the ISR notifies via task notification,
       not through the blusys_lock, so no deadlock.  waiting_task prevents
       a second caller from starting another receive while we are blocked. */
    blusys_lock_give(&rmt_rx->lock);

    notify_val = ulTaskNotifyTake(pdTRUE,
                                  (timeout_ms < 0) ? portMAX_DELAY
                                                   : (TickType_t) pdMS_TO_TICKS(timeout_ms));
    if (notify_val == 0u) {
        rmt_rx->waiting_task = NULL;
        return BLUSYS_ERR_TIMEOUT;
    }

    /* ISR has finished writing pulse_count / symbol_buf — safe to read. */
    size_t sym_count = rmt_rx->pulse_count / 2u;
    size_t out = 0u;

    for (i = 0u; (i < sym_count) && (out < max_pulses); ++i) {
        rmt_symbol_word_t *sym = &rmt_rx->symbol_buf[i];

        if (out < max_pulses) {
            pulses[out].level       = (sym->level0 != 0u);
            pulses[out].duration_us = sym->duration0;
            ++out;
        }
        if ((out < max_pulses) && (sym->duration1 > 0u)) {
            pulses[out].level       = (sym->level1 != 0u);
            pulses[out].duration_us = sym->duration1;
            ++out;
        }
    }

    *out_count = out;
    rmt_rx->waiting_task = NULL;

    return BLUSYS_OK;
}

#else

blusys_err_t blusys_rmt_rx_open(int pin,
                                 const blusys_rmt_rx_config_t *config,
                                 blusys_rmt_rx_t **out_rmt_rx)
{
    (void) pin;
    (void) config;
    (void) out_rmt_rx;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_rmt_rx_close(blusys_rmt_rx_t *rmt_rx)
{
    (void) rmt_rx;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_rmt_rx_read(blusys_rmt_rx_t *rmt_rx,
                                 blusys_rmt_pulse_t *pulses,
                                 size_t max_pulses,
                                 size_t *out_count,
                                 int timeout_ms)
{
    (void) rmt_rx;
    (void) pulses;
    (void) max_pulses;
    (void) out_count;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
