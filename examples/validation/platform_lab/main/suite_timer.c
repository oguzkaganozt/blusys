#include <stdio.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#if CONFIG_BLUSYS_CONCURRENCY_SUITE_TIMER

#define BLUSYS_CONCURRENCY_TIMER_PERIOD_US 10000u  /* 10 ms */
#define BLUSYS_CONCURRENCY_TIMER_ITERS     100u

#define BLUSYS_CONCURRENCY_TIMER_WORKER_A_DONE (1u << 0)
#define BLUSYS_CONCURRENCY_TIMER_WORKER_B_DONE (1u << 1)
#define BLUSYS_CONCURRENCY_TIMER_DONE_MASK \
    (BLUSYS_CONCURRENCY_TIMER_WORKER_A_DONE | BLUSYS_CONCURRENCY_TIMER_WORKER_B_DONE)

typedef struct {
    blusys_timer_t *timer;
    volatile uint32_t tick_count;
    const char *label;
} blusys_concurrency_timer_cb_ctx_t;

typedef struct {
    blusys_timer_t *timer;
    uint32_t errors;
    TaskHandle_t report_task;
    uint32_t done_bit;
} blusys_concurrency_timer_worker_ctx_t;

static bool blusys_concurrency_timer_on_tick_a(blusys_timer_t *timer, void *user_ctx)
{
    blusys_concurrency_timer_cb_ctx_t *ctx = user_ctx;
    (void) timer;
    ctx->tick_count += 1u;
    return false;
}

static bool blusys_concurrency_timer_on_tick_b(blusys_timer_t *timer, void *user_ctx)
{
    blusys_concurrency_timer_cb_ctx_t *ctx = user_ctx;
    (void) timer;
    ctx->tick_count += 1u;
    return false;
}

static bool blusys_concurrency_timer_is_valid_err(blusys_err_t err)
{
    return (err == BLUSYS_OK) ||
           (err == BLUSYS_ERR_INVALID_STATE) ||
           (err == BLUSYS_ERR_BUSY);
}

static void blusys_concurrency_timer_worker_a(void *arg)
{
    blusys_concurrency_timer_worker_ctx_t *ctx = arg;
    uint32_t i;
    blusys_err_t err;

    for (i = 0u; i < BLUSYS_CONCURRENCY_TIMER_ITERS; ++i) {
        err = blusys_timer_stop(ctx->timer);
        if (!blusys_concurrency_timer_is_valid_err(err)) {
            printf("worker_a iter_%u stop unexpected_error: %s\n",
                   (unsigned int) i, blusys_err_string(err));
            ctx->errors += 1u;
        }

        err = blusys_timer_start(ctx->timer);
        if (!blusys_concurrency_timer_is_valid_err(err)) {
            printf("worker_a iter_%u start unexpected_error: %s\n",
                   (unsigned int) i, blusys_err_string(err));
            ctx->errors += 1u;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

static void blusys_concurrency_timer_worker_b(void *arg)
{
    blusys_concurrency_timer_worker_ctx_t *ctx = arg;

    static blusys_concurrency_timer_cb_ctx_t cb_ctx_a = { .timer = NULL, .tick_count = 0u, .label = "cb_a" };
    static blusys_concurrency_timer_cb_ctx_t cb_ctx_b = { .timer = NULL, .tick_count = 0u, .label = "cb_b" };

    uint32_t i;
    blusys_err_t err;

    cb_ctx_a.timer = ctx->timer;
    cb_ctx_b.timer = ctx->timer;

    for (i = 0u; i < BLUSYS_CONCURRENCY_TIMER_ITERS; ++i) {
        blusys_timer_callback_t cb = ((i % 2u) == 0u)
                                         ? blusys_concurrency_timer_on_tick_a
                                         : blusys_concurrency_timer_on_tick_b;
        void *cb_ctx = ((i % 2u) == 0u) ? (void *) &cb_ctx_a : (void *) &cb_ctx_b;

        err = blusys_timer_set_callback(ctx->timer, cb, cb_ctx);
        if (!blusys_concurrency_timer_is_valid_err(err)) {
            printf("worker_b iter_%u set_callback unexpected_error: %s\n",
                   (unsigned int) i, blusys_err_string(err));
            ctx->errors += 1u;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

void run_concurrency_timer_suite(void)
{
    static blusys_concurrency_timer_cb_ctx_t initial_cb_ctx = {
        .timer = NULL,
        .tick_count = 0u,
        .label = "initial",
    };
    blusys_concurrency_timer_worker_ctx_t worker_a = {
        .timer = NULL,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = BLUSYS_CONCURRENCY_TIMER_WORKER_A_DONE,
    };
    blusys_concurrency_timer_worker_ctx_t worker_b = {
        .timer = NULL,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = BLUSYS_CONCURRENCY_TIMER_WORKER_B_DONE,
    };
    blusys_timer_t *timer = NULL;
    blusys_err_t err;
    uint32_t notified_bits = 0u;
    uint32_t received_bits = 0u;

    printf("Blusys concurrency suite — timer\n");
    printf("target: %s\n", blusys_target_name());
    printf("period_us: %u\n", (unsigned int) BLUSYS_CONCURRENCY_TIMER_PERIOD_US);
    printf("iterations_per_worker: %u\n", (unsigned int) BLUSYS_CONCURRENCY_TIMER_ITERS);

    err = blusys_timer_open(BLUSYS_CONCURRENCY_TIMER_PERIOD_US, true, &timer);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    initial_cb_ctx.timer = timer;

    err = blusys_timer_set_callback(timer, blusys_concurrency_timer_on_tick_a, &initial_cb_ctx);
    if (err != BLUSYS_OK) {
        printf("set_callback error: %s\n", blusys_err_string(err));
        blusys_timer_close(timer);
        return;
    }

    err = blusys_timer_start(timer);
    if (err != BLUSYS_OK) {
        printf("start error: %s\n", blusys_err_string(err));
        blusys_timer_close(timer);
        return;
    }

    worker_a.timer = timer;
    worker_b.timer = timer;
    worker_a.report_task = xTaskGetCurrentTaskHandle();
    worker_b.report_task = xTaskGetCurrentTaskHandle();

    xTaskCreate(blusys_concurrency_timer_worker_a, "timer_worker_a", 4096, &worker_a, 5, NULL);
    xTaskCreate(blusys_concurrency_timer_worker_b, "timer_worker_b", 4096, &worker_b, 5, NULL);

    while ((notified_bits & BLUSYS_CONCURRENCY_TIMER_DONE_MASK) != BLUSYS_CONCURRENCY_TIMER_DONE_MASK) {
        xTaskNotifyWait(0u, UINT32_MAX, &received_bits, portMAX_DELAY);
        notified_bits |= received_bits;
    }

    err = blusys_timer_stop(timer);
    if (err != BLUSYS_OK) {
        printf("final stop error: %s\n", blusys_err_string(err));
    }

    printf("worker_a errors: %u\n", (unsigned int) worker_a.errors);
    printf("worker_b errors: %u\n", (unsigned int) worker_b.errors);
    printf("concurrent_result: %s\n",
           ((worker_a.errors == 0u) && (worker_b.errors == 0u)) ? "ok" : "fail");

    err = blusys_timer_close(timer);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}

#else

void run_concurrency_timer_suite(void) {}

#endif /* CONFIG_BLUSYS_CONCURRENCY_SUITE_TIMER */
