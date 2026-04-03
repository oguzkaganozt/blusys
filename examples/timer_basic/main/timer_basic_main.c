#include <stdio.h>

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

#define BLUSYS_TIMER_BASIC_PERIOD_US 500000u
#define BLUSYS_TIMER_BASIC_TICK_LIMIT 5u

typedef struct {
    TaskHandle_t task;
    volatile uint32_t tick_count;
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

void app_main(void)
{
    blusys_timer_basic_ctx_t ctx = {
        .task = xTaskGetCurrentTaskHandle(),
        .tick_count = 0u,
    };
    blusys_timer_t *timer = NULL;
    blusys_err_t err;
    uint32_t printed_ticks = 0u;

    printf("Blusys timer basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("period_us: %u\n", (unsigned int) BLUSYS_TIMER_BASIC_PERIOD_US);

    err = blusys_timer_open(BLUSYS_TIMER_BASIC_PERIOD_US, true, &timer);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_timer_set_callback(timer, blusys_timer_basic_on_tick, &ctx);
    if (err != BLUSYS_OK) {
        printf("set callback error: %s\n", blusys_err_string(err));
        blusys_timer_close(timer);
        return;
    }

    err = blusys_timer_start(timer);
    if (err != BLUSYS_OK) {
        printf("start error: %s\n", blusys_err_string(err));
        blusys_timer_close(timer);
        return;
    }

    while (printed_ticks < BLUSYS_TIMER_BASIC_TICK_LIMIT) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printed_ticks = ctx.tick_count;
        printf("tick=%u\n", (unsigned int) printed_ticks);
    }

    err = blusys_timer_stop(timer);
    if (err != BLUSYS_OK) {
        printf("stop error: %s\n", blusys_err_string(err));
    }

    err = blusys_timer_close(timer);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
