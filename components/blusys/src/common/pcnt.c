#include "blusys/pcnt.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_PCNT_SUPPORTED

#include "blusys_esp_err.h"
#include "blusys_lock.h"

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BLUSYS_PCNT_LOW_LIMIT (-32768)
#define BLUSYS_PCNT_HIGH_LIMIT 32767

struct blusys_pcnt {
    int pin;
    bool started;
    bool closing;
    unsigned int callback_active;
    blusys_pcnt_callback_t callback;
    void *callback_user_ctx;
    pcnt_unit_handle_t unit_handle;
    pcnt_channel_handle_t channel_handle;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_pcnt_state_lock = portMUX_INITIALIZER_UNLOCKED;

static bool blusys_pcnt_is_valid_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

static bool blusys_pcnt_is_valid_edge(blusys_pcnt_edge_t edge)
{
    return (edge >= BLUSYS_PCNT_EDGE_RISING) && (edge <= BLUSYS_PCNT_EDGE_BOTH);
}

static bool blusys_pcnt_callback_active(const blusys_pcnt_t *pcnt)
{
    bool active;

    portENTER_CRITICAL(&blusys_pcnt_state_lock);
    active = pcnt->callback_active > 0u;
    portEXIT_CRITICAL(&blusys_pcnt_state_lock);

    return active;
}

static void blusys_pcnt_wait_for_callback_idle(const blusys_pcnt_t *pcnt)
{
    while (blusys_pcnt_callback_active(pcnt)) {
        taskYIELD();
    }
}

static bool blusys_pcnt_is_started(const blusys_pcnt_t *pcnt)
{
    bool started;

    portENTER_CRITICAL(&blusys_pcnt_state_lock);
    started = pcnt->started;
    portEXIT_CRITICAL(&blusys_pcnt_state_lock);

    return started;
}

static void blusys_pcnt_set_started(blusys_pcnt_t *pcnt, bool started)
{
    portENTER_CRITICAL(&blusys_pcnt_state_lock);
    pcnt->started = started;
    portEXIT_CRITICAL(&blusys_pcnt_state_lock);
}

static void blusys_pcnt_set_closing(blusys_pcnt_t *pcnt, bool closing)
{
    portENTER_CRITICAL(&blusys_pcnt_state_lock);
    pcnt->closing = closing;
    portEXIT_CRITICAL(&blusys_pcnt_state_lock);
}

static bool IRAM_ATTR blusys_pcnt_on_reach(pcnt_unit_handle_t unit,
                                           const pcnt_watch_event_data_t *edata,
                                           void *user_ctx)
{
    blusys_pcnt_t *pcnt = user_ctx;
    blusys_pcnt_callback_t callback;
    void *callback_user_ctx;
    bool closing;
    bool should_yield = false;

    (void) unit;

    portENTER_CRITICAL_ISR(&blusys_pcnt_state_lock);
    pcnt->callback_active += 1u;
    closing = pcnt->closing;
    callback = closing ? NULL : pcnt->callback;
    callback_user_ctx = pcnt->callback_user_ctx;
    portEXIT_CRITICAL_ISR(&blusys_pcnt_state_lock);

    if ((callback != NULL) && (edata != NULL)) {
        should_yield = callback(pcnt, edata->watch_point_value, callback_user_ctx);
    }

    portENTER_CRITICAL_ISR(&blusys_pcnt_state_lock);
    pcnt->callback_active -= 1u;
    portEXIT_CRITICAL_ISR(&blusys_pcnt_state_lock);

    return should_yield;
}

static blusys_err_t blusys_pcnt_set_edge_actions(blusys_pcnt_t *pcnt, blusys_pcnt_edge_t edge)
{
    pcnt_channel_edge_action_t pos_act = PCNT_CHANNEL_EDGE_ACTION_HOLD;
    pcnt_channel_edge_action_t neg_act = PCNT_CHANNEL_EDGE_ACTION_HOLD;
    esp_err_t esp_err;

    switch (edge) {
    case BLUSYS_PCNT_EDGE_RISING:
        pos_act = PCNT_CHANNEL_EDGE_ACTION_INCREASE;
        neg_act = PCNT_CHANNEL_EDGE_ACTION_HOLD;
        break;
    case BLUSYS_PCNT_EDGE_FALLING:
        pos_act = PCNT_CHANNEL_EDGE_ACTION_HOLD;
        neg_act = PCNT_CHANNEL_EDGE_ACTION_INCREASE;
        break;
    case BLUSYS_PCNT_EDGE_BOTH:
        pos_act = PCNT_CHANNEL_EDGE_ACTION_INCREASE;
        neg_act = PCNT_CHANNEL_EDGE_ACTION_INCREASE;
        break;
    default:
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err = pcnt_channel_set_edge_action(pcnt->channel_handle, pos_act, neg_act);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    return blusys_translate_esp_err(pcnt_channel_set_level_action(pcnt->channel_handle,
                                                                  PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                                  PCNT_CHANNEL_LEVEL_ACTION_KEEP));
}

blusys_err_t blusys_pcnt_open(int pin,
                              blusys_pcnt_edge_t edge,
                              uint32_t max_glitch_ns,
                              blusys_pcnt_t **out_pcnt)
{
    blusys_pcnt_t *pcnt;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((out_pcnt == NULL) || !blusys_pcnt_is_valid_pin(pin) || !blusys_pcnt_is_valid_edge(edge)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    pcnt = calloc(1, sizeof(*pcnt));
    if (pcnt == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&pcnt->lock);
    if (err != BLUSYS_OK) {
        free(pcnt);
        return err;
    }

    pcnt->pin = pin;

    esp_err = pcnt_new_unit(&(pcnt_unit_config_t) {
        .low_limit = BLUSYS_PCNT_LOW_LIMIT,
        .high_limit = BLUSYS_PCNT_HIGH_LIMIT,
        .intr_priority = 0,
        .flags.accum_count = 0u,
    }, &pcnt->unit_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = pcnt_new_channel(pcnt->unit_handle,
                               &(pcnt_chan_config_t) {
                                   .edge_gpio_num = pin,
                                   .level_gpio_num = -1,
                                   .flags.invert_edge_input = 0u,
                                   .flags.invert_level_input = 0u,
                                   .flags.virt_edge_io_level = 0u,
                                   .flags.virt_level_io_level = 1u,
                                   .flags.io_loop_back = 0u,
                               },
                               &pcnt->channel_handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_unit;
    }

    err = blusys_pcnt_set_edge_actions(pcnt, edge);
    if (err != BLUSYS_OK) {
        goto fail_channel;
    }

    if (max_glitch_ns > 0u) {
        err = blusys_translate_esp_err(pcnt_unit_set_glitch_filter(pcnt->unit_handle,
                                                                   &(pcnt_glitch_filter_config_t) {
                                                                       .max_glitch_ns = max_glitch_ns,
                                                                   }));
        if (err != BLUSYS_OK) {
            goto fail_channel;
        }
    }

    err = blusys_translate_esp_err(pcnt_unit_register_event_callbacks(pcnt->unit_handle,
                                                                      &(pcnt_event_callbacks_t) {
                                                                          .on_reach = blusys_pcnt_on_reach,
                                                                      },
                                                                      pcnt));
    if (err != BLUSYS_OK) {
        goto fail_channel;
    }

    err = blusys_translate_esp_err(pcnt_unit_enable(pcnt->unit_handle));
    if (err != BLUSYS_OK) {
        goto fail_enabled;
    }

    *out_pcnt = pcnt;
    return BLUSYS_OK;

fail_enabled:
    pcnt_unit_disable(pcnt->unit_handle);
fail_channel:
    pcnt_del_channel(pcnt->channel_handle);
fail_unit:
    pcnt_del_unit(pcnt->unit_handle);
fail_lock:
    blusys_lock_deinit(&pcnt->lock);
    free(pcnt);
    return err;
}

blusys_err_t blusys_pcnt_close(blusys_pcnt_t *pcnt)
{
    blusys_err_t err;

    if (pcnt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_pcnt_set_closing(pcnt, true);

    if (blusys_pcnt_is_started(pcnt)) {
        err = blusys_translate_esp_err(pcnt_unit_stop(pcnt->unit_handle));
        if (err != BLUSYS_OK) {
            goto fail;
        }
        blusys_pcnt_set_started(pcnt, false);
    }

    blusys_pcnt_wait_for_callback_idle(pcnt);

    err = blusys_translate_esp_err(pcnt_unit_disable(pcnt->unit_handle));
    if (err != BLUSYS_OK) {
        goto fail;
    }

    err = blusys_translate_esp_err(pcnt_del_channel(pcnt->channel_handle));
    if (err != BLUSYS_OK) {
        goto fail;
    }

    err = blusys_translate_esp_err(pcnt_del_unit(pcnt->unit_handle));
    if (err != BLUSYS_OK) {
        goto fail;
    }

    blusys_lock_give(&pcnt->lock);
    blusys_lock_deinit(&pcnt->lock);
    free(pcnt);
    return BLUSYS_OK;

fail:
    blusys_pcnt_set_closing(pcnt, false);
    blusys_lock_give(&pcnt->lock);
    return err;
}

blusys_err_t blusys_pcnt_start(blusys_pcnt_t *pcnt)
{
    blusys_err_t err;

    if (pcnt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_pcnt_is_started(pcnt)) {
        blusys_lock_give(&pcnt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(pcnt_unit_start(pcnt->unit_handle));
    if (err == BLUSYS_OK) {
        blusys_pcnt_set_started(pcnt, true);
    }

    blusys_lock_give(&pcnt->lock);
    return err;
}

blusys_err_t blusys_pcnt_stop(blusys_pcnt_t *pcnt)
{
    blusys_err_t err;

    if (pcnt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!blusys_pcnt_is_started(pcnt)) {
        blusys_lock_give(&pcnt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(pcnt_unit_stop(pcnt->unit_handle));
    if (err == BLUSYS_OK) {
        blusys_pcnt_set_started(pcnt, false);
    }

    /* Release lock before the spin-wait — the unit is already stopped so no
       new watch-point callbacks can fire; this drains any in-flight one. */
    blusys_lock_give(&pcnt->lock);

    if (err == BLUSYS_OK) {
        blusys_pcnt_wait_for_callback_idle(pcnt);
    }

    return err;
}

blusys_err_t blusys_pcnt_clear_count(blusys_pcnt_t *pcnt)
{
    blusys_err_t err;

    if (pcnt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_pcnt_is_started(pcnt)) {
        blusys_lock_give(&pcnt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(pcnt_unit_clear_count(pcnt->unit_handle));

    blusys_lock_give(&pcnt->lock);
    return err;
}

blusys_err_t blusys_pcnt_get_count(blusys_pcnt_t *pcnt, int *out_count)
{
    blusys_err_t err;

    if ((pcnt == NULL) || (out_count == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(pcnt_unit_get_count(pcnt->unit_handle, out_count));

    blusys_lock_give(&pcnt->lock);
    return err;
}

blusys_err_t blusys_pcnt_add_watch_point(blusys_pcnt_t *pcnt, int count)
{
    blusys_err_t err;

    if ((pcnt == NULL) || (count < BLUSYS_PCNT_LOW_LIMIT) || (count > BLUSYS_PCNT_HIGH_LIMIT)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_pcnt_is_started(pcnt)) {
        blusys_lock_give(&pcnt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(pcnt_unit_add_watch_point(pcnt->unit_handle, count));
    if (err == BLUSYS_OK) {
        err = blusys_translate_esp_err(pcnt_unit_clear_count(pcnt->unit_handle));
    }

    blusys_lock_give(&pcnt->lock);
    return err;
}

blusys_err_t blusys_pcnt_remove_watch_point(blusys_pcnt_t *pcnt, int count)
{
    blusys_err_t err;

    if ((pcnt == NULL) || (count < BLUSYS_PCNT_LOW_LIMIT) || (count > BLUSYS_PCNT_HIGH_LIMIT)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_pcnt_is_started(pcnt)) {
        blusys_lock_give(&pcnt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(pcnt_unit_remove_watch_point(pcnt->unit_handle, count));
    if (err == BLUSYS_OK) {
        err = blusys_translate_esp_err(pcnt_unit_clear_count(pcnt->unit_handle));
    }

    blusys_lock_give(&pcnt->lock);
    return err;
}

blusys_err_t blusys_pcnt_set_callback(blusys_pcnt_t *pcnt,
                                      blusys_pcnt_callback_t callback,
                                      void *user_ctx)
{
    blusys_err_t err;

    if (pcnt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pcnt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_pcnt_is_started(pcnt)) {
        blusys_lock_give(&pcnt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_pcnt_wait_for_callback_idle(pcnt);
    pcnt->callback = callback;
    pcnt->callback_user_ctx = user_ctx;

    blusys_lock_give(&pcnt->lock);
    return BLUSYS_OK;
}
#else
blusys_err_t blusys_pcnt_open(int pin,
                              blusys_pcnt_edge_t edge,
                              uint32_t max_glitch_ns,
                              blusys_pcnt_t **out_pcnt)
{
    (void) pin;
    (void) edge;
    (void) max_glitch_ns;

    if (out_pcnt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_pcnt = NULL;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_close(blusys_pcnt_t *pcnt)
{
    (void) pcnt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_start(blusys_pcnt_t *pcnt)
{
    (void) pcnt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_stop(blusys_pcnt_t *pcnt)
{
    (void) pcnt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_clear_count(blusys_pcnt_t *pcnt)
{
    (void) pcnt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_get_count(blusys_pcnt_t *pcnt, int *out_count)
{
    (void) pcnt;
    (void) out_count;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_add_watch_point(blusys_pcnt_t *pcnt, int count)
{
    (void) pcnt;
    (void) count;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_remove_watch_point(blusys_pcnt_t *pcnt, int count)
{
    (void) pcnt;
    (void) count;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_pcnt_set_callback(blusys_pcnt_t *pcnt,
                                      blusys_pcnt_callback_t callback,
                                      void *user_ctx)
{
    (void) pcnt;
    (void) callback;
    (void) user_ctx;
    return BLUSYS_ERR_NOT_SUPPORTED;
}
#endif
