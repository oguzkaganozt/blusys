#include <stdio.h>

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

#define BLUSYS_TIMER_BASIC_PERIOD_US 500000u

#define BLUSYS_TIMER_BASIC_PHASE_A_TICKS 3u
#define BLUSYS_TIMER_BASIC_PHASE_B_TICKS 2u

typedef struct {
    TaskHandle_t task;
    volatile uint32_t tick_count;
    const char *label;
} blusys_timer_basic_ctx_t;

static bool blusys_timer_basic_on_tick(blusys_timer_t *timer, void *user_ctx)
{
    blusys_timer_basic_ctx_t *ctx = user_ctx;
    BaseType_t high_task_woken = pdFALSE;

    (void) timer;

    ctx->tick_count += 1u;
    vTaskNotifyGiveFromISR(ctx->task, &high_task_woken);

    return high_task_woken == pdTRUE;
}

static bool blusys_timer_basic_run_phase(blusys_timer_t *timer,
                                         blusys_timer_basic_ctx_t *ctx,
                                         uint32_t tick_limit)
{
    blusys_err_t err;
    uint32_t printed_ticks = 0u;

    ctx->tick_count = 0u;

    err = blusys_timer_set_callback(timer, blusys_timer_basic_on_tick, ctx);
    if (err != BLUSYS_OK) {
        printf("%s set callback error: %s\n", ctx->label, blusys_err_string(err));
        return false;
    }

    err = blusys_timer_start(timer);
    if (err != BLUSYS_OK) {
        printf("%s start error: %s\n", ctx->label, blusys_err_string(err));
        return false;
    }

    while (printed_ticks < tick_limit) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printed_ticks = ctx->tick_count;
        printf("%s tick=%u\n", ctx->label, (unsigned int) printed_ticks);
    }

    err = blusys_timer_stop(timer);
    if (err != BLUSYS_OK) {
        printf("%s stop error: %s\n", ctx->label, blusys_err_string(err));
        return false;
    }

    printf("%s result: %s\n", ctx->label, (ctx->tick_count == tick_limit) ? "ok" : "incomplete");
    return ctx->tick_count == tick_limit;
}

void app_main(void)
{
    blusys_timer_basic_ctx_t phase_a = {
        .task = xTaskGetCurrentTaskHandle(),
        .tick_count = 0u,
        .label = "phase_a",
    };
    blusys_timer_basic_ctx_t phase_b = {
        .task = xTaskGetCurrentTaskHandle(),
        .tick_count = 0u,
        .label = "phase_b",
    };
    blusys_timer_t *timer = NULL;
    blusys_err_t err;
    bool phase_a_ok;
    bool phase_b_ok;

    printf("Blusys timer basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("period_us: %u\n", (unsigned int) BLUSYS_TIMER_BASIC_PERIOD_US);

    err = blusys_timer_open(BLUSYS_TIMER_BASIC_PERIOD_US, true, &timer);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    phase_a_ok = blusys_timer_basic_run_phase(timer, &phase_a, BLUSYS_TIMER_BASIC_PHASE_A_TICKS);
    phase_b_ok = blusys_timer_basic_run_phase(timer, &phase_b, BLUSYS_TIMER_BASIC_PHASE_B_TICKS);

    printf("callback_swap_result: %s\n", (phase_a_ok && phase_b_ok) ? "ok" : "incomplete");

    err = blusys_timer_close(timer);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
