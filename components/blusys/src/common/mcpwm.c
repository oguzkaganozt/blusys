#include "blusys/mcpwm.h"

#include "soc/soc_caps.h"

#if SOC_MCPWM_SUPPORTED

#include <stddef.h>
#include <stdlib.h>

#include "blusys_esp_err.h"
#include "blusys_lock.h"

#include "driver/gpio.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "freertos/FreeRTOS.h"

#define BLUSYS_MCPWM_MAX_HANDLES   3
#define BLUSYS_MCPWM_RESOLUTION_HZ 10000000u   /* 10 MHz → 100 ns per tick */

struct blusys_mcpwm {
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t  oper;
    mcpwm_cmpr_handle_t  cmpr;
    mcpwm_gen_handle_t   gen_a;
    mcpwm_gen_handle_t   gen_b;
    uint32_t             period_ticks;
    blusys_lock_t        lock;
    int                  slot;
};

static portMUX_TYPE s_slot_mux = portMUX_INITIALIZER_UNLOCKED;
static bool         s_slot_in_use[BLUSYS_MCPWM_MAX_HANDLES];

static int alloc_slot(void)
{
    int slot = -1;

    portENTER_CRITICAL(&s_slot_mux);
    for (int i = 0; i < BLUSYS_MCPWM_MAX_HANDLES; ++i) {
        if (!s_slot_in_use[i]) {
            s_slot_in_use[i] = true;
            slot = i;
            break;
        }
    }
    portEXIT_CRITICAL(&s_slot_mux);

    return slot;
}

static void free_slot(int slot)
{
    if ((slot < 0) || (slot >= BLUSYS_MCPWM_MAX_HANDLES)) {
        return;
    }

    portENTER_CRITICAL(&s_slot_mux);
    s_slot_in_use[slot] = false;
    portEXIT_CRITICAL(&s_slot_mux);
}

blusys_err_t blusys_mcpwm_open(int pin_a,
                               int pin_b,
                               uint32_t freq_hz,
                               uint16_t duty_permille,
                               uint32_t dead_time_ns,
                               blusys_mcpwm_t **out_mcpwm)
{
    blusys_mcpwm_t *h;
    blusys_err_t err;
    esp_err_t esp_err;
    int slot;
    uint32_t period_ticks;
    uint32_t duty_ticks;
    uint32_t dead_ticks;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin_a) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(pin_b) ||
        (freq_hz == 0u) ||
        (duty_permille > 1000u) ||
        (out_mcpwm == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    slot = alloc_slot();
    if (slot < 0) {
        return BLUSYS_ERR_BUSY;
    }

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        free_slot(slot);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        free_slot(slot);
        return err;
    }

    h->slot = slot;
    period_ticks = BLUSYS_MCPWM_RESOLUTION_HZ / freq_hz;
    duty_ticks = (duty_permille * period_ticks) / 1000u;
    h->period_ticks = period_ticks;

    /* Timer */
    mcpwm_timer_config_t timer_cfg = {
        .group_id      = slot,
        .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = BLUSYS_MCPWM_RESOLUTION_HZ,
        .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks  = period_ticks,
    };
    esp_err = mcpwm_new_timer(&timer_cfg, &h->timer);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    /* Operator */
    mcpwm_operator_config_t oper_cfg = {
        .group_id = slot,
    };
    esp_err = mcpwm_new_operator(&oper_cfg, &h->oper);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_timer;
    }

    esp_err = mcpwm_operator_connect_timer(h->oper, h->timer);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_oper;
    }

    /* Comparator */
    mcpwm_comparator_config_t cmpr_cfg = {
        .flags.update_cmp_on_tez = true,
    };
    esp_err = mcpwm_new_comparator(h->oper, &cmpr_cfg, &h->cmpr);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_oper;
    }

    esp_err = mcpwm_comparator_set_compare_value(h->cmpr, duty_ticks);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_cmpr;
    }

    /* Generators */
    mcpwm_generator_config_t gen_a_cfg = { .gen_gpio_num = pin_a };
    esp_err = mcpwm_new_generator(h->oper, &gen_a_cfg, &h->gen_a);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_cmpr;
    }

    mcpwm_generator_config_t gen_b_cfg = { .gen_gpio_num = pin_b };
    esp_err = mcpwm_new_generator(h->oper, &gen_b_cfg, &h->gen_b);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_gen_a;
    }

    /* gen_a: high on timer empty (zero), low on comparator match */
    esp_err = mcpwm_generator_set_action_on_timer_event(
        h->gen_a,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                     MCPWM_TIMER_EVENT_EMPTY,
                                     MCPWM_GEN_ACTION_HIGH));
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_gen_b;
    }

    esp_err = mcpwm_generator_set_action_on_compare_event(
        h->gen_a,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                       h->cmpr,
                                       MCPWM_GEN_ACTION_LOW));
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_gen_b;
    }

    if (dead_time_ns == 0u) {
        /* gen_b: inverse of gen_a via generator actions */
        esp_err = mcpwm_generator_set_action_on_timer_event(
            h->gen_b,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                         MCPWM_TIMER_EVENT_EMPTY,
                                         MCPWM_GEN_ACTION_LOW));
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_gen_b;
        }

        esp_err = mcpwm_generator_set_action_on_compare_event(
            h->gen_b,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                           h->cmpr,
                                           MCPWM_GEN_ACTION_HIGH));
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_gen_b;
        }
    } else {
        /* Dead-time: rising delay on gen_a, falling delay + invert on gen_b */
        dead_ticks = (uint32_t) ((uint64_t) dead_time_ns *
                                 BLUSYS_MCPWM_RESOLUTION_HZ / 1000000000u);

        mcpwm_dead_time_config_t dt_a = {
            .posedge_delay_ticks = dead_ticks,
            .negedge_delay_ticks = 0u,
        };
        esp_err = mcpwm_generator_set_dead_time(h->gen_a, h->gen_a, &dt_a);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_gen_b;
        }

        mcpwm_dead_time_config_t dt_b = {
            .posedge_delay_ticks = 0u,
            .negedge_delay_ticks = dead_ticks,
            .flags.invert_output = true,
        };
        esp_err = mcpwm_generator_set_dead_time(h->gen_a, h->gen_b, &dt_b);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_gen_b;
        }
    }

    /* Start timer */
    esp_err = mcpwm_timer_enable(h->timer);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_gen_b;
    }

    esp_err = mcpwm_timer_start_stop(h->timer, MCPWM_TIMER_START_NO_STOP);
    if (esp_err != ESP_OK) {
        mcpwm_timer_disable(h->timer);
        err = blusys_translate_esp_err(esp_err);
        goto fail_gen_b;
    }

    *out_mcpwm = h;
    return BLUSYS_OK;

fail_gen_b:
    mcpwm_del_generator(h->gen_b);
fail_gen_a:
    mcpwm_del_generator(h->gen_a);
fail_cmpr:
    mcpwm_del_comparator(h->cmpr);
fail_oper:
    mcpwm_del_operator(h->oper);
fail_timer:
    mcpwm_del_timer(h->timer);
fail_lock:
    blusys_lock_deinit(&h->lock);
    free(h);
    free_slot(slot);
    return err;
}

blusys_err_t blusys_mcpwm_close(blusys_mcpwm_t *mcpwm)
{
    blusys_err_t err;

    if (mcpwm == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&mcpwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    mcpwm_timer_start_stop(mcpwm->timer, MCPWM_TIMER_START_STOP_EMPTY);
    vTaskDelay(pdMS_TO_TICKS(1));   /* wait for current period to complete before disabling */
    mcpwm_timer_disable(mcpwm->timer);
    mcpwm_del_generator(mcpwm->gen_b);
    mcpwm_del_generator(mcpwm->gen_a);
    mcpwm_del_comparator(mcpwm->cmpr);
    mcpwm_del_operator(mcpwm->oper);
    mcpwm_del_timer(mcpwm->timer);

    blusys_lock_give(&mcpwm->lock);
    blusys_lock_deinit(&mcpwm->lock);
    free_slot(mcpwm->slot);
    free(mcpwm);

    return BLUSYS_OK;
}

blusys_err_t blusys_mcpwm_set_duty(blusys_mcpwm_t *mcpwm, uint16_t duty_permille)
{
    blusys_err_t err;
    uint32_t duty_ticks;

    if ((mcpwm == NULL) || (duty_permille > 1000u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&mcpwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    duty_ticks = (duty_permille * mcpwm->period_ticks) / 1000u;
    err = blusys_translate_esp_err(
        mcpwm_comparator_set_compare_value(mcpwm->cmpr, duty_ticks));

    blusys_lock_give(&mcpwm->lock);
    return err;
}

#else

blusys_err_t blusys_mcpwm_open(int pin_a,
                               int pin_b,
                               uint32_t freq_hz,
                               uint16_t duty_permille,
                               uint32_t dead_time_ns,
                               blusys_mcpwm_t **out_mcpwm)
{
    (void) pin_a;
    (void) pin_b;
    (void) freq_hz;
    (void) duty_permille;
    (void) dead_time_ns;
    (void) out_mcpwm;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mcpwm_close(blusys_mcpwm_t *mcpwm)
{
    (void) mcpwm;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mcpwm_set_duty(blusys_mcpwm_t *mcpwm, uint16_t duty_permille)
{
    (void) mcpwm;
    (void) duty_permille;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
