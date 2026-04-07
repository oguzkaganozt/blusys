#include "blusys/one_wire.h"

#include "soc/soc_caps.h"

#if SOC_RMT_SUPPORTED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ──────────────────────────────────────────────── Timing constants (1 µs/tick) */

#define BLUSYS_OW_RESOLUTION_HZ          1000000u

/* Reset pulse */
#define BLUSYS_OW_RESET_LOW_US           480u
#define BLUSYS_OW_RESET_RELEASE_US       70u     /* time TX holds HIGH before device responds */
#define BLUSYS_OW_RESET_IDLE_NS          480000u /* idle after presence → RX done; satisfies recovery */
#define BLUSYS_OW_PRESENCE_MIN_US        60u
#define BLUSYS_OW_PRESENCE_MAX_US        300u

/* Write slots */
#define BLUSYS_OW_WRITE1_LOW_US          6u
#define BLUSYS_OW_WRITE1_HIGH_US         64u
#define BLUSYS_OW_WRITE0_LOW_US          60u
#define BLUSYS_OW_WRITE0_HIGH_US         10u

/* Read slots */
#define BLUSYS_OW_READ_LOW_US            6u
#define BLUSYS_OW_READ_HIGH_US           64u
#define BLUSYS_OW_READ_BIT_THRESHOLD_US  15u     /* LOW ≤ this → bit 1; LOW > this → bit 0 */
#define BLUSYS_OW_READ_IDLE_NS           200000u /* idle after last slot → RX done */

/* RX channel */
#define BLUSYS_OW_RX_BUF_SYMBOLS        64u
#define BLUSYS_OW_RX_GLITCH_NS          1000u   /* ignore glitches shorter than 1 µs */

/* Timeouts */
#define BLUSYS_OW_TX_TIMEOUT_MS         50
#define BLUSYS_OW_RX_TIMEOUT_MS         100

/* ROM commands */
#define BLUSYS_OW_CMD_READ_ROM          0x33u
#define BLUSYS_OW_CMD_SKIP_ROM          0xCCu
#define BLUSYS_OW_CMD_MATCH_ROM         0x55u
#define BLUSYS_OW_CMD_SEARCH_ROM        0xF0u

/* Shared TX config: idle HIGH after every transmission, no loop */
static const rmt_transmit_config_t s_ow_tx_cfg = {
    .loop_count      = 0,
    .flags.eot_level = 1u,
};

/* Read-initiation slot: master drives LOW for 6 µs then releases */
static const rmt_symbol_word_t s_ow_read_init_sym = {
    .level0    = 0u,
    .duration0 = BLUSYS_OW_READ_LOW_US,
    .level1    = 1u,
    .duration1 = BLUSYS_OW_READ_HIGH_US,
};

/* ──────────────────────────────────────────────────────────────── Handle */

struct blusys_one_wire {
    bool                 closing;
    rmt_channel_handle_t tx_channel;
    rmt_encoder_handle_t tx_encoder;
    rmt_channel_handle_t rx_channel;
    rmt_symbol_word_t   *rx_sym_buf;       /* heap buffer passed to rmt_receive() */
    volatile size_t      rx_sym_count;     /* written by ISR; read after task notification */
    volatile TaskHandle_t waiting_task;    /* non-NULL = bus busy; also RX notification target */
    /* SEARCH ROM state (Maxim AN187) */
    uint8_t              search_rom[8];
    int                  search_last_discrepancy;
    bool                 search_last_device_flag;
    blusys_lock_t        lock;
};

/* ──────────────────────────────────────────────────── ISR callback */

static IRAM_ATTR bool blusys_ow_rx_done_cb(rmt_channel_handle_t channel,
                                             const rmt_rx_done_event_data_t *edata,
                                             void *user_ctx)
{
    blusys_one_wire_t *ow = (blusys_one_wire_t *) user_ctx;
    BaseType_t higher_prio_woken = pdFALSE;

    (void) channel;

    ow->rx_sym_count = edata->num_symbols;

    if (ow->waiting_task != NULL) {
        vTaskNotifyGiveFromISR(ow->waiting_task, &higher_prio_woken);
        ow->waiting_task = NULL;
    }

    return higher_prio_woken == pdTRUE;
}

/* ──────────────────────────────────────────────── Internal helpers */

/*
 * Arm RX to capture one transaction, then start TX.
 * LOCK MUST BE HELD by caller. Releases lock before returning.
 * Caller must call blusys_ow_rx_wait() immediately after.
 *
 * waiting_task must be NULL when entering (checked inside).
 */
static blusys_err_t blusys_ow_arm_and_tx(blusys_one_wire_t *ow,
                                          const rmt_symbol_word_t *tx_syms,
                                          size_t tx_sym_count,
                                          uint32_t rx_range_max_ns)
{
    esp_err_t esp_err;

    if (ow->waiting_task != NULL) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    ow->rx_sym_count  = 0u;
    ow->waiting_task  = xTaskGetCurrentTaskHandle();

    /* Arm RX before TX so no transitions are missed */
    esp_err = rmt_receive(ow->rx_channel,
                          ow->rx_sym_buf,
                          BLUSYS_OW_RX_BUF_SYMBOLS * sizeof(rmt_symbol_word_t),
                          &(rmt_receive_config_t) {
                              .signal_range_min_ns = BLUSYS_OW_RX_GLITCH_NS,
                              .signal_range_max_ns = rx_range_max_ns,
                          });
    if (esp_err != ESP_OK) {
        ow->waiting_task = NULL;
        blusys_lock_give(&ow->lock);
        return blusys_translate_esp_err(esp_err);
    }

    esp_err = rmt_transmit(ow->tx_channel,
                           ow->tx_encoder,
                           tx_syms,
                           tx_sym_count * sizeof(rmt_symbol_word_t),
                           &s_ow_tx_cfg);
    if (esp_err != ESP_OK) {
        ow->waiting_task = NULL;
        rmt_disable(ow->rx_channel);    /* abort pending receive */
        rmt_enable(ow->rx_channel);
        blusys_lock_give(&ow->lock);
        return blusys_translate_esp_err(esp_err);
    }

    /* Release lock — ISR notifies via task notification, not through the lock */
    blusys_lock_give(&ow->lock);
    return BLUSYS_OK;
}

/* Block until the ISR fires from a blusys_ow_arm_and_tx() call. */
static blusys_err_t blusys_ow_rx_wait(blusys_one_wire_t *ow)
{
    uint32_t val = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLUSYS_OW_RX_TIMEOUT_MS));

    if (val == 0u) {
        ow->waiting_task = NULL;
        return BLUSYS_ERR_TIMEOUT;
    }
    return BLUSYS_OK;
}

/*
 * Transmit without using RX (write-only operations).
 * LOCK MUST BE HELD by caller. Releases lock before blocking.
 * Blocks until TX is complete; no lock held during the wait.
 *
 * Uses waiting_task as a bus-busy guard — any concurrent caller
 * that takes the lock will see waiting_task != NULL and return
 * BLUSYS_ERR_INVALID_STATE.
 */
static blusys_err_t blusys_ow_tx_only(blusys_one_wire_t *ow,
                                       const rmt_symbol_word_t *tx_syms,
                                       size_t tx_sym_count)
{
    esp_err_t esp_err;
    blusys_err_t err;

    if (ow->waiting_task != NULL) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    ow->waiting_task = xTaskGetCurrentTaskHandle();

    esp_err = rmt_transmit(ow->tx_channel,
                           ow->tx_encoder,
                           tx_syms,
                           tx_sym_count * sizeof(rmt_symbol_word_t),
                           &s_ow_tx_cfg);
    if (esp_err != ESP_OK) {
        ow->waiting_task = NULL;
        blusys_lock_give(&ow->lock);
        return blusys_translate_esp_err(esp_err);
    }

    /* Release lock before blocking; rmt_tx_wait_all_done does not use task notifications */
    blusys_lock_give(&ow->lock);

    esp_err = rmt_tx_wait_all_done(ow->tx_channel, BLUSYS_OW_TX_TIMEOUT_MS);
    err = blusys_translate_esp_err(esp_err);

    ow->waiting_task = NULL;
    return err;
}

/* ──────────────────────────────────────────────────────── Lifecycle */

blusys_err_t blusys_one_wire_open(int pin,
                                   const blusys_one_wire_config_t *config,
                                   blusys_one_wire_t **out_ow)
{
    blusys_one_wire_t *ow;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin) || (config == NULL) || (out_ow == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    ow = calloc(1, sizeof(*ow));
    if (ow == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    ow->rx_sym_buf = calloc(BLUSYS_OW_RX_BUF_SYMBOLS, sizeof(*ow->rx_sym_buf));
    if (ow->rx_sym_buf == NULL) {
        free(ow);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&ow->lock);
    if (err != BLUSYS_OK) {
        free(ow->rx_sym_buf);
        free(ow);
        return err;
    }

    /* TX channel: open-drain is mandatory for 1-Wire; idle HIGH */
    esp_err = rmt_new_tx_channel(&(rmt_tx_channel_config_t) {
        .gpio_num          = (gpio_num_t) pin,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = BLUSYS_OW_RESOLUTION_HZ,
        .mem_block_symbols = 64u,
        .trans_queue_depth = 1u,
        .intr_priority     = 0,
        .flags.invert_out  = 0u,
        .flags.with_dma    = 0u,
        .flags.io_loop_back = 0u,
        .flags.io_od_mode  = 1u,   /* open-drain: allows devices to pull bus LOW */
        .flags.allow_pd    = 0u,
        .flags.init_level  = 1u,   /* idle HIGH */
    }, &ow->tx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = rmt_new_copy_encoder(&(rmt_copy_encoder_config_t) {}, &ow->tx_encoder);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tx_channel;
    }

    /* RX channel: same GPIO; reads the actual bus state during read/reset operations */
    esp_err = rmt_new_rx_channel(&(rmt_rx_channel_config_t) {
        .gpio_num          = (gpio_num_t) pin,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = BLUSYS_OW_RESOLUTION_HZ,
        .mem_block_symbols = BLUSYS_OW_RX_BUF_SYMBOLS,
        .intr_priority     = 0,
    }, &ow->rx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tx_encoder;
    }

    esp_err = rmt_rx_register_event_callbacks(ow->rx_channel,
                                              &(rmt_rx_event_callbacks_t) {
                                                  .on_recv_done = blusys_ow_rx_done_cb,
                                              },
                                              ow);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_rx_channel;
    }

    esp_err = rmt_enable(ow->tx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_rx_channel;
    }

    esp_err = rmt_enable(ow->rx_channel);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tx_enable;
    }

    *out_ow = ow;
    return BLUSYS_OK;

fail_tx_enable:
    rmt_disable(ow->tx_channel);
fail_rx_channel:
    rmt_del_channel(ow->rx_channel);
fail_tx_encoder:
    rmt_del_encoder(ow->tx_encoder);
fail_tx_channel:
    rmt_del_channel(ow->tx_channel);
fail_lock:
    blusys_lock_deinit(&ow->lock);
    free(ow->rx_sym_buf);
    free(ow);
    return err;
}

blusys_err_t blusys_one_wire_close(blusys_one_wire_t *ow)
{
    blusys_err_t err;

    if (ow == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&ow->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    ow->closing = true;

    rmt_disable(ow->rx_channel);
    rmt_del_channel(ow->rx_channel);
    rmt_disable(ow->tx_channel);
    rmt_del_encoder(ow->tx_encoder);
    rmt_del_channel(ow->tx_channel);

    blusys_lock_give(&ow->lock);
    blusys_lock_deinit(&ow->lock);
    free(ow->rx_sym_buf);
    free(ow);
    return BLUSYS_OK;
}

/* ──────────────────────────────────────────────────────── Bus primitives */

blusys_err_t blusys_one_wire_reset(blusys_one_wire_t *ow)
{
    blusys_err_t err;
    bool present;
    size_t i;

    if (ow == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&ow->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (ow->closing) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    /* One symbol: LOW for 480 µs, then release HIGH for 70 µs (presence window opens) */
    static const rmt_symbol_word_t reset_sym = {
        .level0    = 0u,
        .duration0 = BLUSYS_OW_RESET_LOW_US,
        .level1    = 1u,
        .duration1 = BLUSYS_OW_RESET_RELEASE_US,
    };

    /* blusys_ow_arm_and_tx releases the lock before returning */
    err = blusys_ow_arm_and_tx(ow, &reset_sym, 1u, BLUSYS_OW_RESET_IDLE_NS);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_ow_rx_wait(ow);
    if (err != BLUSYS_OK) {
        return err;
    }

    /*
     * Detect presence pulse: a LOW symbol with duration 60–300 µs.
     * The 480 µs TX reset pulse is filtered out by the minimum check.
     */
    present = false;
    for (i = 0u; i < ow->rx_sym_count; ++i) {
        const rmt_symbol_word_t *s = &ow->rx_sym_buf[i];
        if ((s->level0 == 0u) &&
            (s->duration0 >= BLUSYS_OW_PRESENCE_MIN_US) &&
            (s->duration0 <= BLUSYS_OW_PRESENCE_MAX_US)) {
            present = true;
            break;
        }
        if ((s->level1 == 0u) &&
            (s->duration1 >= BLUSYS_OW_PRESENCE_MIN_US) &&
            (s->duration1 <= BLUSYS_OW_PRESENCE_MAX_US)) {
            present = true;
            break;
        }
    }

    return present ? BLUSYS_OK : BLUSYS_ERR_NOT_FOUND;
}

blusys_err_t blusys_one_wire_write_bit(blusys_one_wire_t *ow, bool bit)
{
    blusys_err_t err;

    if (ow == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&ow->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (ow->closing) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    const rmt_symbol_word_t sym = {
        .level0    = 0u,
        .duration0 = bit ? BLUSYS_OW_WRITE1_LOW_US : BLUSYS_OW_WRITE0_LOW_US,
        .level1    = 1u,
        .duration1 = bit ? BLUSYS_OW_WRITE1_HIGH_US : BLUSYS_OW_WRITE0_HIGH_US,
    };

    return blusys_ow_tx_only(ow, &sym, 1u);
}

blusys_err_t blusys_one_wire_read_bit(blusys_one_wire_t *ow, bool *out_bit)
{
    blusys_err_t err;

    if ((ow == NULL) || (out_bit == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&ow->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (ow->closing) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    /*
     * TX drives LOW for 6 µs (read initiation), then releases (open-drain HIGH).
     * Device responds by holding LOW for ~45 µs (bit 0) or releasing immediately (bit 1).
     * RX captures: LOW duration ≤ 15 µs → bus went HIGH quickly → bit 1.
     *              LOW duration > 15 µs → device held LOW → bit 0.
     */
    err = blusys_ow_arm_and_tx(ow, &s_ow_read_init_sym, 1u, BLUSYS_OW_READ_IDLE_NS);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_ow_rx_wait(ow);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (ow->rx_sym_count == 0u) {
        return BLUSYS_ERR_IO;
    }

    *out_bit = (ow->rx_sym_buf[0].duration0 <= BLUSYS_OW_READ_BIT_THRESHOLD_US);
    return BLUSYS_OK;
}

blusys_err_t blusys_one_wire_write_byte(blusys_one_wire_t *ow, uint8_t byte)
{
    blusys_err_t err;
    rmt_symbol_word_t syms[8];
    int i;

    if (ow == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&ow->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (ow->closing) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    for (i = 0; i < 8; ++i) {
        bool bit = (bool) ((byte >> i) & 1u);   /* LSB first */
        syms[i] = (rmt_symbol_word_t) {
            .level0    = 0u,
            .duration0 = bit ? BLUSYS_OW_WRITE1_LOW_US : BLUSYS_OW_WRITE0_LOW_US,
            .level1    = 1u,
            .duration1 = bit ? BLUSYS_OW_WRITE1_HIGH_US : BLUSYS_OW_WRITE0_HIGH_US,
        };
    }

    return blusys_ow_tx_only(ow, syms, 8u);
}

blusys_err_t blusys_one_wire_read_byte(blusys_one_wire_t *ow, uint8_t *out_byte)
{
    blusys_err_t err;
    rmt_symbol_word_t syms[8];
    uint8_t result;
    int i;

    if ((ow == NULL) || (out_byte == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&ow->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (ow->closing) {
        blusys_lock_give(&ow->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    /*
     * Send 8 identical read-initiation slots in a single rmt_transmit call.
     * RX captures all 8 slots; each captures 1 symbol per bit.
     * The 200 µs idle threshold fires after the last slot completes.
     */
    for (i = 0; i < 8; ++i) {
        syms[i] = s_ow_read_init_sym;
    }

    err = blusys_ow_arm_and_tx(ow, syms, 8u, BLUSYS_OW_READ_IDLE_NS);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_ow_rx_wait(ow);
    if (err != BLUSYS_OK) {
        return err;
    }

    result = 0u;
    for (i = 0; (i < 8) && ((size_t) i < ow->rx_sym_count); ++i) {
        bool bit = (ow->rx_sym_buf[i].duration0 <= BLUSYS_OW_READ_BIT_THRESHOLD_US);
        result |= (uint8_t)(bit ? (1u << i) : 0u);
    }

    *out_byte = result;
    return BLUSYS_OK;
}

blusys_err_t blusys_one_wire_write(blusys_one_wire_t *ow, const uint8_t *data, size_t len)
{
    blusys_err_t err;
    size_t i;

    if ((ow == NULL) || (data == NULL) || (len == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    for (i = 0u; i < len; ++i) {
        err = blusys_one_wire_write_byte(ow, data[i]);
        if (err != BLUSYS_OK) {
            return err;
        }
    }
    return BLUSYS_OK;
}

blusys_err_t blusys_one_wire_read(blusys_one_wire_t *ow, uint8_t *data, size_t len)
{
    blusys_err_t err;
    size_t i;

    if ((ow == NULL) || (data == NULL) || (len == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    for (i = 0u; i < len; ++i) {
        err = blusys_one_wire_read_byte(ow, &data[i]);
        if (err != BLUSYS_OK) {
            return err;
        }
    }
    return BLUSYS_OK;
}

/* ──────────────────────────────────────────────────────── ROM commands */

blusys_err_t blusys_one_wire_read_rom(blusys_one_wire_t *ow,
                                       blusys_one_wire_rom_t *out_rom)
{
    blusys_err_t err;

    if ((ow == NULL) || (out_rom == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_one_wire_reset(ow);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_one_wire_write_byte(ow, BLUSYS_OW_CMD_READ_ROM);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_one_wire_read(ow, out_rom->bytes, 8u);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_one_wire_crc8(out_rom->bytes, 7u) != out_rom->bytes[7]) {
        return BLUSYS_ERR_IO;
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_one_wire_skip_rom(blusys_one_wire_t *ow)
{
    blusys_err_t err;

    if (ow == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_one_wire_reset(ow);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_one_wire_write_byte(ow, BLUSYS_OW_CMD_SKIP_ROM);
}

blusys_err_t blusys_one_wire_match_rom(blusys_one_wire_t *ow,
                                        const blusys_one_wire_rom_t *rom)
{
    blusys_err_t err;

    if ((ow == NULL) || (rom == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_one_wire_reset(ow);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_one_wire_write_byte(ow, BLUSYS_OW_CMD_MATCH_ROM);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_one_wire_write(ow, rom->bytes, 8u);
}

blusys_err_t blusys_one_wire_search_reset(blusys_one_wire_t *ow)
{
    if (ow == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    memset(ow->search_rom, 0, sizeof(ow->search_rom));
    ow->search_last_discrepancy  = 0;
    ow->search_last_device_flag  = false;
    return BLUSYS_OK;
}

blusys_err_t blusys_one_wire_search(blusys_one_wire_t *ow,
                                     blusys_one_wire_rom_t *out_rom,
                                     bool *out_last_device)
{
    blusys_err_t err;
    uint8_t new_rom[8];
    int last_zero;
    int id_bit_number;

    if ((ow == NULL) || (out_rom == NULL) || (out_last_device == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (ow->search_last_device_flag) {
        blusys_one_wire_search_reset(ow);
        return BLUSYS_ERR_NOT_FOUND;
    }

    err = blusys_one_wire_reset(ow);
    if (err != BLUSYS_OK) {
        blusys_one_wire_search_reset(ow);
        return err;
    }

    err = blusys_one_wire_write_byte(ow, BLUSYS_OW_CMD_SEARCH_ROM);
    if (err != BLUSYS_OK) {
        blusys_one_wire_search_reset(ow);
        return err;
    }

    memset(new_rom, 0, sizeof(new_rom));
    last_zero = 0;

    /*
     * Standard Maxim SEARCH ROM algorithm (Application Note 187).
     * Iterates all 64 bit positions; each step reads two complement bits from
     * the bus (all devices simultaneously), then writes back the chosen direction.
     */
    for (id_bit_number = 1; id_bit_number <= 64; ++id_bit_number) {
        int byte_idx = (id_bit_number - 1) / 8;
        int bit_mask = 1 << ((id_bit_number - 1) % 8);
        bool id_bit, cmp_id_bit;
        int search_direction;

        err = blusys_one_wire_read_bit(ow, &id_bit);
        if (err != BLUSYS_OK) {
            blusys_one_wire_search_reset(ow);
            return err;
        }
        err = blusys_one_wire_read_bit(ow, &cmp_id_bit);
        if (err != BLUSYS_OK) {
            blusys_one_wire_search_reset(ow);
            return err;
        }

        if (id_bit && cmp_id_bit) {
            /* Both complement bits 1: no devices present or bus error */
            blusys_one_wire_search_reset(ow);
            return BLUSYS_ERR_NOT_FOUND;
        }

        if (id_bit != cmp_id_bit) {
            /* All devices agree on this bit value */
            search_direction = id_bit ? 1 : 0;
        } else {
            /* Discrepancy: navigate based on previous search path */
            if (id_bit_number < ow->search_last_discrepancy) {
                search_direction = (ow->search_rom[byte_idx] & bit_mask) ? 1 : 0;
            } else if (id_bit_number == ow->search_last_discrepancy) {
                search_direction = 1;
            } else {
                search_direction = 0;
            }
            if (search_direction == 0) {
                last_zero = id_bit_number;
            }
        }

        if (search_direction) {
            new_rom[byte_idx] |= (uint8_t) bit_mask;
        }

        err = blusys_one_wire_write_bit(ow, (bool) search_direction);
        if (err != BLUSYS_OK) {
            blusys_one_wire_search_reset(ow);
            return err;
        }
    }

    if (blusys_one_wire_crc8(new_rom, 7u) != new_rom[7]) {
        blusys_one_wire_search_reset(ow);
        return BLUSYS_ERR_IO;
    }

    memcpy(ow->search_rom, new_rom, 8u);
    ow->search_last_discrepancy = last_zero;
    ow->search_last_device_flag = (last_zero == 0);

    memcpy(out_rom->bytes, new_rom, 8u);
    *out_last_device = ow->search_last_device_flag;
    return BLUSYS_OK;
}

/* ──────────────────────────────────────────────────────── Utility */

uint8_t blusys_one_wire_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0u;
    size_t i;
    size_t b;

    for (i = 0u; i < len; ++i) {
        uint8_t byte = data[i];
        for (b = 0u; b < 8u; ++b) {
            uint8_t mix = (crc ^ byte) & 0x01u;
            crc >>= 1u;
            if (mix != 0u) {
                crc ^= 0x8Cu;   /* Dallas/Maxim polynomial 0x31, bit-reflected */
            }
            byte >>= 1u;
        }
    }
    return crc;
}

/* ──────────────────────────────────────────────── Stubs (no RMT) */

#else /* SOC_RMT_SUPPORTED */

blusys_err_t blusys_one_wire_open(int pin,
                                   const blusys_one_wire_config_t *config,
                                   blusys_one_wire_t **out_ow)
{
    (void) pin; (void) config; (void) out_ow;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_close(blusys_one_wire_t *ow)
{
    (void) ow;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_reset(blusys_one_wire_t *ow)
{
    (void) ow;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_write_bit(blusys_one_wire_t *ow, bool bit)
{
    (void) ow; (void) bit;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_read_bit(blusys_one_wire_t *ow, bool *out_bit)
{
    (void) ow; (void) out_bit;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_write_byte(blusys_one_wire_t *ow, uint8_t byte)
{
    (void) ow; (void) byte;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_read_byte(blusys_one_wire_t *ow, uint8_t *out_byte)
{
    (void) ow; (void) out_byte;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_write(blusys_one_wire_t *ow, const uint8_t *data, size_t len)
{
    (void) ow; (void) data; (void) len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_read(blusys_one_wire_t *ow, uint8_t *data, size_t len)
{
    (void) ow; (void) data; (void) len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_read_rom(blusys_one_wire_t *ow,
                                       blusys_one_wire_rom_t *out_rom)
{
    (void) ow; (void) out_rom;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_skip_rom(blusys_one_wire_t *ow)
{
    (void) ow;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_match_rom(blusys_one_wire_t *ow,
                                        const blusys_one_wire_rom_t *rom)
{
    (void) ow; (void) rom;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_search(blusys_one_wire_t *ow,
                                     blusys_one_wire_rom_t *out_rom,
                                     bool *out_last_device)
{
    (void) ow; (void) out_rom; (void) out_last_device;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_one_wire_search_reset(blusys_one_wire_t *ow)
{
    (void) ow;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

uint8_t blusys_one_wire_crc8(const uint8_t *data, size_t len)
{
    (void) data; (void) len;
    return 0u;
}

#endif /* SOC_RMT_SUPPORTED */
