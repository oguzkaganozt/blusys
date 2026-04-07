#include "blusys/sensor/dht.h"

#include "soc/soc_caps.h"

#if SOC_RMT_SUPPORTED

#include <stdlib.h>
#include <string.h>

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#include "driver/gpio.h"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "hal/rmt_types.h"

#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ──────────────────────────────────────────────── Timing constants (1 µs/tick) */

#define DHT_RESOLUTION_HZ          1000000u   /* 1 MHz → 1 µs per tick */

/* Start pulse: host pulls LOW to wake the sensor */
#define DHT_START_LOW_US           20000u      /* 20 ms — satisfies both DHT11 (≥18 ms) and DHT22 (≥1 ms) */
#define DHT_START_RELEASE_US       40u         /* host releases; sensor responds within 20-40 µs */

/* RX configuration */
#define DHT_RX_BUF_SYMBOLS        41u         /* 1 response symbol + 40 data symbols = 41 max */
#define DHT_RX_GLITCH_NS          1000u       /* 1 µs glitch filter */
#define DHT_RX_IDLE_NS            200000u     /* 200 µs idle → frame complete */

/* Bit decoding threshold */
#define DHT_BIT_THRESHOLD_US       40u         /* HIGH > 40 µs → bit 1; HIGH ≤ 40 µs → bit 0 */

/* Timeouts */
#define DHT_RX_TIMEOUT_MS          100

/* Minimum interval between reads (µs) */
#define DHT_MIN_INTERVAL_US        2000000     /* 2 s — conservative for both DHT11 and DHT22 */

/* Data frame */
#define DHT_DATA_BITS              40u
#define DHT_DATA_BYTES             5u

/* TX config: idle HIGH (open-drain release) after transmission */
static const rmt_transmit_config_t s_dht_tx_cfg = {
    .loop_count      = 0,
    .flags.eot_level = 1u,
};

/* ──────────────────────────────────────────────────────────────── Handle */

struct blusys_dht {
    blusys_dht_type_t    type;
    bool                 closing;

    rmt_channel_handle_t tx_channel;
    rmt_encoder_handle_t tx_encoder;
    rmt_channel_handle_t rx_channel;
    rmt_symbol_word_t   *rx_sym_buf;
    volatile size_t      rx_sym_count;
    volatile TaskHandle_t waiting_task;

    int64_t              last_read_us;   /* 0 = no reading yet; >0 = µs timestamp of last read */
    blusys_dht_reading_t last_reading;

    blusys_lock_t        lock;
};

/* ──────────────────────────────────────────────────── ISR callback */

static IRAM_ATTR bool dht_rx_done_cb(rmt_channel_handle_t channel,
                                      const rmt_rx_done_event_data_t *edata,
                                      void *user_ctx)
{
    blusys_dht_t *dht = (blusys_dht_t *) user_ctx;
    BaseType_t higher_prio_woken = pdFALSE;

    (void) channel;

    dht->rx_sym_count = edata->num_symbols;

    if (dht->waiting_task != NULL) {
        vTaskNotifyGiveFromISR(dht->waiting_task, &higher_prio_woken);
        dht->waiting_task = NULL;
    }

    return higher_prio_woken == pdTRUE;
}

/* ──────────────────────────────────────────────── Data decoding */

/*
 * Decode RMT symbols into 5 data bytes.
 *
 * Expected frame:
 *   sym[0]: DHT response — LOW ~80 µs, HIGH ~80 µs
 *   sym[1..40]: data bits — LOW ~50 µs, HIGH 26-28 µs (0) or 70 µs (1)
 *
 * Each RMT symbol contains two pulses: {level0, duration0, level1, duration1}.
 * For DHT data bits: level0=LOW (start of bit), level1=HIGH (data).
 */
static blusys_err_t dht_decode(const rmt_symbol_word_t *symbols,
                                size_t symbol_count,
                                uint8_t data[DHT_DATA_BYTES])
{
    /* Need at least 1 response symbol + 40 data symbols */
    if (symbol_count < 1u + DHT_DATA_BITS) {
        return BLUSYS_ERR_IO;
    }

    memset(data, 0, DHT_DATA_BYTES);

    /* Skip the first symbol (response pulse), decode 40 data bits */
    for (size_t i = 0; i < DHT_DATA_BITS; i++) {
        const rmt_symbol_word_t *sym = &symbols[1u + i];

        /*
         * level0 = LOW (bit start), duration0 = ~50 µs
         * level1 = HIGH (data), duration1 = 26-28 µs (0) or ~70 µs (1)
         */
        if (sym->duration1 > DHT_BIT_THRESHOLD_US) {
            data[i / 8u] |= (uint8_t)(1u << (7u - (i % 8u)));
        }
    }

    /* Verify checksum: sum of first 4 bytes must equal byte 5 (mod 256) */
    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        return BLUSYS_ERR_IO;
    }

    return BLUSYS_OK;
}

/*
 * Convert raw bytes to temperature and humidity.
 *
 * DHT11:  data[0] = humidity integer, data[1] = humidity decimal
 *         data[2] = temp integer,     data[3] = temp decimal
 *
 * DHT22:  data[0..1] = humidity × 10 (16-bit unsigned)
 *         data[2..3] = temperature × 10 (16-bit; bit 15 = sign)
 */
static void dht_convert(blusys_dht_type_t type,
                          const uint8_t data[DHT_DATA_BYTES],
                          blusys_dht_reading_t *out)
{
    if (type == BLUSYS_DHT_TYPE_DHT11) {
        out->humidity_pct    = (float)data[0] + (float)data[1] * 0.1f;
        out->temperature_c   = (float)data[2] + (float)data[3] * 0.1f;
    } else {
        uint16_t raw_hum  = ((uint16_t)data[0] << 8u) | data[1];
        uint16_t raw_temp = ((uint16_t)data[2] << 8u) | data[3];

        out->humidity_pct = (float)raw_hum * 0.1f;

        bool negative = (raw_temp & 0x8000u) != 0;
        raw_temp &= 0x7FFFu;
        out->temperature_c = (float)raw_temp * 0.1f;
        if (negative) {
            out->temperature_c = -out->temperature_c;
        }
    }
}

/* ──────────────────────────────────────────────────────── Lifecycle */

blusys_err_t blusys_dht_open(const blusys_dht_config_t *config,
                              blusys_dht_t **out_handle)
{
    blusys_dht_t *dht;
    blusys_err_t err;
    esp_err_t esp_err;

    if (config == NULL || out_handle == NULL ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    dht = calloc(1, sizeof(*dht));
    if (dht == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    dht->rx_sym_buf = calloc(DHT_RX_BUF_SYMBOLS, sizeof(*dht->rx_sym_buf));
    if (dht->rx_sym_buf == NULL) {
        free(dht);
        return BLUSYS_ERR_NO_MEM;
    }

    dht->type = config->type;

    err = blusys_lock_init(&dht->lock);
    if (err != BLUSYS_OK) {
        free(dht->rx_sym_buf);
        free(dht);
        return err;
    }

    /* TX channel: open-drain, idle HIGH — allows sensor to pull bus LOW */
    esp_err = rmt_new_tx_channel(&(rmt_tx_channel_config_t) {
        .gpio_num           = (gpio_num_t) config->pin,
        .clk_src            = RMT_CLK_SRC_DEFAULT,
        .resolution_hz      = DHT_RESOLUTION_HZ,
        .mem_block_symbols  = 64u,
        .trans_queue_depth  = 1u,
        .intr_priority      = 0,
        .flags.invert_out   = 0u,
        .flags.with_dma     = 0u,
        .flags.io_loop_back = 0u,
        .flags.io_od_mode   = 1u,      /* open-drain */
        .flags.allow_pd     = 0u,
        .flags.init_level   = 1u,      /* idle HIGH */
    }, &dht->tx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = rmt_new_copy_encoder(&(rmt_copy_encoder_config_t) {}, &dht->tx_encoder);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tx_channel;
    }

    /* RX channel: same GPIO — captures the sensor response */
    esp_err = rmt_new_rx_channel(&(rmt_rx_channel_config_t) {
        .gpio_num           = (gpio_num_t) config->pin,
        .clk_src            = RMT_CLK_SRC_DEFAULT,
        .resolution_hz      = DHT_RESOLUTION_HZ,
        .mem_block_symbols  = DHT_RX_BUF_SYMBOLS,
        .intr_priority      = 0,
    }, &dht->rx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tx_encoder;
    }

    esp_err = rmt_rx_register_event_callbacks(dht->rx_channel,
                                              &(rmt_rx_event_callbacks_t) {
                                                  .on_recv_done = dht_rx_done_cb,
                                              },
                                              dht);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_rx_channel;
    }

    esp_err = rmt_enable(dht->tx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_rx_channel;
    }

    esp_err = rmt_enable(dht->rx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tx_enable;
    }

    *out_handle = dht;
    return BLUSYS_OK;

fail_tx_enable:
    rmt_disable(dht->tx_channel);
fail_rx_channel:
    rmt_del_channel(dht->rx_channel);
fail_tx_encoder:
    rmt_del_encoder(dht->tx_encoder);
fail_tx_channel:
    rmt_del_channel(dht->tx_channel);
fail_lock:
    blusys_lock_deinit(&dht->lock);
    free(dht->rx_sym_buf);
    free(dht);
    return err;
}

blusys_err_t blusys_dht_close(blusys_dht_t *dht)
{
    blusys_err_t err;

    if (dht == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dht->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    dht->closing = true;

    rmt_disable(dht->rx_channel);
    rmt_del_channel(dht->rx_channel);
    rmt_disable(dht->tx_channel);
    rmt_del_encoder(dht->tx_encoder);
    rmt_del_channel(dht->tx_channel);

    blusys_lock_give(&dht->lock);
    blusys_lock_deinit(&dht->lock);
    free(dht->rx_sym_buf);
    free(dht);
    return BLUSYS_OK;
}

/* ──────────────────────────────────────────────────────── Read */

blusys_err_t blusys_dht_read(blusys_dht_t *dht,
                              blusys_dht_reading_t *out_reading)
{
    blusys_err_t err;
    esp_err_t esp_err;
    uint8_t data[DHT_DATA_BYTES];

    if (dht == NULL || out_reading == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dht->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (dht->closing) {
        blusys_lock_give(&dht->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    /* Rate-limit: return cached reading if called too soon */
    int64_t now_us = esp_timer_get_time();
    if (dht->last_read_us > 0 && (now_us - dht->last_read_us) < DHT_MIN_INTERVAL_US) {
        *out_reading = dht->last_reading;
        blusys_lock_give(&dht->lock);
        return BLUSYS_OK;
    }

    if (dht->waiting_task != NULL) {
        blusys_lock_give(&dht->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    /* Prepare for RX capture */
    dht->rx_sym_count = 0u;
    dht->waiting_task = xTaskGetCurrentTaskHandle();

    /* Arm RX before TX so no transitions are missed */
    esp_err = rmt_receive(dht->rx_channel,
                          dht->rx_sym_buf,
                          DHT_RX_BUF_SYMBOLS * sizeof(rmt_symbol_word_t),
                          &(rmt_receive_config_t) {
                              .signal_range_min_ns = DHT_RX_GLITCH_NS,
                              .signal_range_max_ns = DHT_RX_IDLE_NS,
                          });
    if (esp_err != ESP_OK) {
        dht->waiting_task = NULL;
        blusys_lock_give(&dht->lock);
        return blusys_translate_esp_err(esp_err);
    }

    /* Send start pulse: LOW for 20 ms, then release HIGH */
    static const rmt_symbol_word_t start_sym = {
        .level0    = 0u,
        .duration0 = DHT_START_LOW_US,
        .level1    = 1u,
        .duration1 = DHT_START_RELEASE_US,
    };

    esp_err = rmt_transmit(dht->tx_channel,
                           dht->tx_encoder,
                           &start_sym,
                           sizeof(start_sym),
                           &s_dht_tx_cfg);
    if (esp_err != ESP_OK) {
        dht->waiting_task = NULL;
        rmt_disable(dht->rx_channel);
        rmt_enable(dht->rx_channel);
        blusys_lock_give(&dht->lock);
        return blusys_translate_esp_err(esp_err);
    }

    /* Release lock before blocking — ISR notifies via task notification */
    blusys_lock_give(&dht->lock);

    /* Wait for RX completion */
    uint32_t val = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(DHT_RX_TIMEOUT_MS));
    if (val == 0u) {
        dht->waiting_task = NULL;
        /* Abort the armed RX channel so the next read starts clean */
        rmt_disable(dht->rx_channel);
        rmt_enable(dht->rx_channel);
        return BLUSYS_ERR_TIMEOUT;
    }

    /* Decode the captured symbols */
    err = dht_decode(dht->rx_sym_buf, dht->rx_sym_count, data);
    if (err != BLUSYS_OK) {
        return err;
    }

    /* Convert raw bytes to floating-point values */
    dht_convert(dht->type, data, out_reading);

    /* Cache the reading — timestamp records when sampling completed */
    dht->last_reading = *out_reading;
    dht->last_read_us = esp_timer_get_time();

    return BLUSYS_OK;
}

#else /* !SOC_RMT_SUPPORTED */

blusys_err_t blusys_dht_open(const blusys_dht_config_t *config,
                              blusys_dht_t **out_handle)
{
    (void) config;
    (void) out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_dht_close(blusys_dht_t *dht)
{
    (void) dht;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_dht_read(blusys_dht_t *dht,
                              blusys_dht_reading_t *out_reading)
{
    (void) dht;
    (void) out_reading;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
