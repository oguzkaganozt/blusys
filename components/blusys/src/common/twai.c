#include "blusys/twai.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_TWAI_SUPPORTED

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/twai_types.h"

#define BLUSYS_TWAI_MAX_DATA_LEN 8u
#define BLUSYS_TWAI_FILTER_ID 0u
#define BLUSYS_TWAI_SAMPLE_POINT_PERMILL 800u
#define BLUSYS_TWAI_TX_QUEUE_DEPTH 1u
#define BLUSYS_TWAI_FAIL_RETRY_COUNT 3

struct blusys_twai {
    int tx_pin;
    int rx_pin;
    uint32_t bitrate;
    bool started;
    bool closing;
    unsigned int rx_callback_active;
    blusys_twai_rx_callback_t rx_callback;
    void *rx_callback_user_ctx;
    TaskHandle_t tx_wait_task;
    bool tx_result_ready;
    bool tx_result_success;
    twai_node_handle_t node_handle;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_twai_state_lock = portMUX_INITIALIZER_UNLOCKED;

static bool blusys_twai_is_valid_tx_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_twai_is_valid_rx_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

static bool blusys_twai_is_valid_bitrate(uint32_t bitrate)
{
    return bitrate > 0u;
}

static bool blusys_twai_frame_id_is_valid(const blusys_twai_frame_t *frame)
{
    return frame->id <= TWAI_STD_ID_MASK;
}

static bool blusys_twai_is_valid_frame(const blusys_twai_frame_t *frame)
{
    if (frame == NULL) {
        return false;
    }

    if (!blusys_twai_frame_id_is_valid(frame)) {
        return false;
    }

    if (frame->data_len > BLUSYS_TWAI_MAX_DATA_LEN) {
        return false;
    }

    if (frame->dlc > BLUSYS_TWAI_MAX_DATA_LEN) {
        return false;
    }

    if (frame->remote_frame) {
        return frame->data_len == 0u;
    }

    if (frame->dlc != frame->data_len) {
        return false;
    }

    return true;
}

static bool blusys_twai_callback_active(const blusys_twai_t *twai)
{
    bool active;

    portENTER_CRITICAL(&blusys_twai_state_lock);
    active = twai->rx_callback_active > 0u;
    portEXIT_CRITICAL(&blusys_twai_state_lock);

    return active;
}

static void blusys_twai_wait_for_callback_idle(const blusys_twai_t *twai)
{
    while (blusys_twai_callback_active(twai)) {
        taskYIELD();
    }
}

static bool blusys_twai_is_started(const blusys_twai_t *twai)
{
    bool started;

    portENTER_CRITICAL(&blusys_twai_state_lock);
    started = twai->started;
    portEXIT_CRITICAL(&blusys_twai_state_lock);

    return started;
}

static void blusys_twai_set_started(blusys_twai_t *twai, bool started)
{
    portENTER_CRITICAL(&blusys_twai_state_lock);
    twai->started = started;
    portEXIT_CRITICAL(&blusys_twai_state_lock);
}

static void blusys_twai_set_closing(blusys_twai_t *twai, bool closing)
{
    portENTER_CRITICAL(&blusys_twai_state_lock);
    twai->closing = closing;
    portEXIT_CRITICAL(&blusys_twai_state_lock);
}

static void blusys_twai_clear_tx_wait(blusys_twai_t *twai)
{
    portENTER_CRITICAL(&blusys_twai_state_lock);
    twai->tx_wait_task = NULL;
    twai->tx_result_ready = false;
    twai->tx_result_success = false;
    portEXIT_CRITICAL(&blusys_twai_state_lock);
}

static blusys_err_t blusys_twai_reset_started_node(blusys_twai_t *twai)
{
    blusys_err_t err;

    err = blusys_translate_esp_err(twai_node_disable(twai->node_handle));
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_translate_esp_err(twai_node_enable(twai->node_handle));
}

static bool IRAM_ATTR blusys_twai_on_tx_done(twai_node_handle_t handle,
                                             const twai_tx_done_event_data_t *edata,
                                             void *user_ctx)
{
    blusys_twai_t *twai = user_ctx;
    TaskHandle_t wait_task;
    BaseType_t should_yield = pdFALSE;

    (void) handle;

    portENTER_CRITICAL_ISR(&blusys_twai_state_lock);
    wait_task = twai->tx_wait_task;
    if (wait_task != NULL) {
        twai->tx_result_ready = true;
        twai->tx_result_success = (edata != NULL) && edata->is_tx_success;
        twai->tx_wait_task = NULL;
    }
    portEXIT_CRITICAL_ISR(&blusys_twai_state_lock);

    if (wait_task != NULL) {
        vTaskNotifyGiveFromISR(wait_task, &should_yield);
    }

    return should_yield == pdTRUE;
}

static bool IRAM_ATTR blusys_twai_on_rx_done(twai_node_handle_t handle,
                                             const twai_rx_done_event_data_t *edata,
                                             void *user_ctx)
{
    blusys_twai_t *twai = user_ctx;
    blusys_twai_rx_callback_t callback;
    void *callback_user_ctx;
    blusys_twai_frame_t frame = {0};
    twai_frame_t rx_frame = {0};
    uint8_t data[BLUSYS_TWAI_MAX_DATA_LEN];
    bool should_yield = false;

    (void) edata;

    portENTER_CRITICAL_ISR(&blusys_twai_state_lock);
    if (twai->closing || (twai->rx_callback == NULL)) {
        portEXIT_CRITICAL_ISR(&blusys_twai_state_lock);
        return false;
    }

    twai->rx_callback_active += 1u;
    callback = twai->rx_callback;
    callback_user_ctx = twai->rx_callback_user_ctx;
    portEXIT_CRITICAL_ISR(&blusys_twai_state_lock);

    rx_frame.buffer = data;
    rx_frame.buffer_len = sizeof(data);
    if (twai_node_receive_from_isr(handle, &rx_frame) == ESP_OK) {
        size_t data_len = (rx_frame.header.dlc > BLUSYS_TWAI_MAX_DATA_LEN) ? BLUSYS_TWAI_MAX_DATA_LEN : rx_frame.header.dlc;

        frame.id = rx_frame.header.id;
        frame.remote_frame = rx_frame.header.rtr;
        frame.dlc = (uint8_t) data_len;
        frame.data_len = frame.remote_frame ? 0u : data_len;
        if (frame.data_len > 0u) {
            memcpy(frame.data, data, frame.data_len);
        }
        should_yield = callback(twai, &frame, callback_user_ctx);
    }

    portENTER_CRITICAL_ISR(&blusys_twai_state_lock);
    twai->rx_callback_active -= 1u;
    portEXIT_CRITICAL_ISR(&blusys_twai_state_lock);

    return should_yield;
}

blusys_err_t blusys_twai_open(int tx_pin,
                              int rx_pin,
                              uint32_t bitrate,
                              blusys_twai_t **out_twai)
{
    blusys_twai_t *twai;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((out_twai == NULL) || !blusys_twai_is_valid_tx_pin(tx_pin) || !blusys_twai_is_valid_rx_pin(rx_pin) ||
        !blusys_twai_is_valid_bitrate(bitrate)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    twai = calloc(1, sizeof(*twai));
    if (twai == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&twai->lock);
    if (err != BLUSYS_OK) {
        free(twai);
        return err;
    }

    twai->tx_pin = tx_pin;
    twai->rx_pin = rx_pin;
    twai->bitrate = bitrate;

    esp_err = twai_new_node_onchip(&(twai_onchip_node_config_t) {
        .io_cfg = {
            .tx = tx_pin,
            .rx = rx_pin,
            .quanta_clk_out = -1,
            .bus_off_indicator = -1,
        },
        .clk_src = TWAI_CLK_SRC_DEFAULT,
        .bit_timing = {
            .bitrate = bitrate,
            .sp_permill = BLUSYS_TWAI_SAMPLE_POINT_PERMILL,
            .ssp_permill = 0u,
        },
        .data_timing = {0},
        .fail_retry_cnt = BLUSYS_TWAI_FAIL_RETRY_COUNT,
        .tx_queue_depth = BLUSYS_TWAI_TX_QUEUE_DEPTH,
        .intr_priority = 0,
        .flags = {
            .enable_self_test = 0u,
            .enable_loopback = 0u,
            .enable_listen_only = 0u,
            .no_receive_rtr = 0u,
        },
    }, &twai->node_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = twai_node_config_mask_filter(twai->node_handle,
                                           BLUSYS_TWAI_FILTER_ID,
                                           &(twai_mask_filter_config_t) {
                                               .id = 0u,
                                               .mask = 0u,
                                               .is_ext = false,
                                               .no_classic = false,
                                               .no_fd = true,
                                               .dual_filter = false,
                                           });
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_node;
    }

    esp_err = twai_node_register_event_callbacks(twai->node_handle,
                                                 &(twai_event_callbacks_t) {
                                                     .on_tx_done = blusys_twai_on_tx_done,
                                                     .on_rx_done = blusys_twai_on_rx_done,
                                                     .on_state_change = NULL,
                                                     .on_error = NULL,
                                                 },
                                                 twai);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_node;
    }

    *out_twai = twai;
    return BLUSYS_OK;

fail_node:
    twai_node_delete(twai->node_handle);
fail_lock:
    blusys_lock_deinit(&twai->lock);
    free(twai);
    return err;
}

blusys_err_t blusys_twai_close(blusys_twai_t *twai)
{
    blusys_err_t err;

    if (twai == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&twai->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_twai_set_closing(twai, true);
    blusys_twai_clear_tx_wait(twai);

    if (blusys_twai_is_started(twai)) {
        err = blusys_translate_esp_err(twai_node_disable(twai->node_handle));
        if (err != BLUSYS_OK) {
            goto fail;
        }
        blusys_twai_set_started(twai, false);
    }

    blusys_twai_wait_for_callback_idle(twai);

    err = blusys_translate_esp_err(twai_node_delete(twai->node_handle));
    if (err != BLUSYS_OK) {
        goto fail;
    }

    blusys_lock_give(&twai->lock);
    blusys_lock_deinit(&twai->lock);
    free(twai);
    return BLUSYS_OK;

fail:
    blusys_twai_set_closing(twai, false);
    blusys_lock_give(&twai->lock);
    return err;
}

blusys_err_t blusys_twai_start(blusys_twai_t *twai)
{
    blusys_err_t err;

    if (twai == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&twai->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (twai->closing || blusys_twai_is_started(twai)) {
        blusys_lock_give(&twai->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(twai_node_enable(twai->node_handle));
    if (err == BLUSYS_OK) {
        blusys_twai_set_started(twai, true);
    }

    blusys_lock_give(&twai->lock);
    return err;
}

blusys_err_t blusys_twai_stop(blusys_twai_t *twai)
{
    blusys_err_t err;

    if (twai == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&twai->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!blusys_twai_is_started(twai)) {
        blusys_lock_give(&twai->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(twai_node_disable(twai->node_handle));
    if (err == BLUSYS_OK) {
        blusys_twai_set_started(twai, false);
    }

    blusys_lock_give(&twai->lock);
    return err;
}

blusys_err_t blusys_twai_write(blusys_twai_t *twai,
                               const blusys_twai_frame_t *frame,
                               int timeout_ms)
{
    blusys_err_t err;
    twai_frame_t tx_frame;
    uint32_t notified;

    if ((twai == NULL) || !blusys_twai_is_valid_frame(frame) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&twai->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (twai->closing || !blusys_twai_is_started(twai)) {
        blusys_lock_give(&twai->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    tx_frame.header.id = frame->id;
    tx_frame.header.dlc = frame->dlc;
    tx_frame.header.ide = 0u;
    tx_frame.header.rtr = frame->remote_frame;
    tx_frame.header.fdf = 0u;
    tx_frame.header.brs = 0u;
    tx_frame.header.esi = 0u;
    tx_frame.buffer = frame->remote_frame ? NULL : (uint8_t *) frame->data;
    tx_frame.buffer_len = frame->data_len;

    while (ulTaskNotifyTake(pdTRUE, 0) > 0u) {
    }

    portENTER_CRITICAL(&blusys_twai_state_lock);
    twai->tx_wait_task = xTaskGetCurrentTaskHandle();
    twai->tx_result_ready = false;
    twai->tx_result_success = false;
    portEXIT_CRITICAL(&blusys_twai_state_lock);

    err = blusys_translate_esp_err(twai_node_transmit(twai->node_handle, &tx_frame, timeout_ms));
    if (err != BLUSYS_OK) {
        blusys_twai_clear_tx_wait(twai);
        blusys_lock_give(&twai->lock);
        return err;
    }

    notified = ulTaskNotifyTake(pdTRUE, blusys_timeout_ms_to_ticks(timeout_ms));
    if (notified == 0u) {
        blusys_twai_clear_tx_wait(twai);
        err = blusys_twai_reset_started_node(twai);
        blusys_lock_give(&twai->lock);
        return (err == BLUSYS_OK) ? BLUSYS_ERR_TIMEOUT : err;
    }

    portENTER_CRITICAL(&blusys_twai_state_lock);
    err = twai->tx_result_ready ? (twai->tx_result_success ? BLUSYS_OK : BLUSYS_ERR_IO) : BLUSYS_ERR_INTERNAL;
    twai->tx_result_ready = false;
    twai->tx_result_success = false;
    portEXIT_CRITICAL(&blusys_twai_state_lock);

    blusys_lock_give(&twai->lock);
    return err;
}

blusys_err_t blusys_twai_set_rx_callback(blusys_twai_t *twai,
                                         blusys_twai_rx_callback_t callback,
                                         void *user_ctx)
{
    blusys_err_t err;

    if (twai == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&twai->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (twai->closing) {
        blusys_lock_give(&twai->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    portENTER_CRITICAL(&blusys_twai_state_lock);
    twai->rx_callback = callback;
    twai->rx_callback_user_ctx = user_ctx;
    portEXIT_CRITICAL(&blusys_twai_state_lock);

    blusys_lock_give(&twai->lock);
    return BLUSYS_OK;
}

#else
blusys_err_t blusys_twai_open(int tx_pin,
                              int rx_pin,
                              uint32_t bitrate,
                              blusys_twai_t **out_twai)
{
    (void) tx_pin;
    (void) rx_pin;
    (void) bitrate;

    if (out_twai == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_twai = NULL;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_twai_close(blusys_twai_t *twai)
{
    (void) twai;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_twai_start(blusys_twai_t *twai)
{
    (void) twai;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_twai_stop(blusys_twai_t *twai)
{
    (void) twai;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_twai_write(blusys_twai_t *twai,
                               const blusys_twai_frame_t *frame,
                               int timeout_ms)
{
    (void) twai;
    (void) frame;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_twai_set_rx_callback(blusys_twai_t *twai,
                                         blusys_twai_rx_callback_t callback,
                                         void *user_ctx)
{
    (void) twai;
    (void) callback;
    (void) user_ctx;
    return BLUSYS_ERR_NOT_SUPPORTED;
}
#endif
