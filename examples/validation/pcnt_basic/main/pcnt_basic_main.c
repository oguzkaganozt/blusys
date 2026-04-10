#include <stdio.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_PCNT_BASIC_PWM_PIN CONFIG_BLUSYS_PCNT_BASIC_PWM_PIN
#define BLUSYS_PCNT_BASIC_INPUT_PIN CONFIG_BLUSYS_PCNT_BASIC_INPUT_PIN

#define BLUSYS_PCNT_BASIC_FREQ_HZ 2000u
#define BLUSYS_PCNT_BASIC_DUTY_PERMILLE 500u
#define BLUSYS_PCNT_BASIC_GLITCH_NS 1000u

#define BLUSYS_PCNT_BASIC_PHASE_A_LIMIT 1000
#define BLUSYS_PCNT_BASIC_PHASE_B_LIMIT 600

typedef struct {
    QueueHandle_t queue;
} blusys_pcnt_basic_ctx_t;

static bool blusys_pcnt_basic_on_watch(blusys_pcnt_t *pcnt, int watch_point, void *user_ctx)
{
    blusys_pcnt_basic_ctx_t *ctx = user_ctx;
    BaseType_t high_task_woken = pdFALSE;

    (void) pcnt;

    xQueueSendFromISR(ctx->queue, &watch_point, &high_task_woken);
    return high_task_woken == pdTRUE;
}

static void blusys_pcnt_basic_drain_watch_queue(blusys_pcnt_basic_ctx_t *ctx)
{
    int watch_point;

    while (xQueueReceive(ctx->queue, &watch_point, 0) == pdTRUE) {
        printf("watch_point=%d\n", watch_point);
    }
}

static bool blusys_pcnt_basic_run_phase(blusys_pcnt_t *pcnt,
                                        blusys_pcnt_basic_ctx_t *ctx,
                                        const char *label,
                                        int limit)
{
    blusys_err_t err;
    int count = 0;

    err = blusys_pcnt_clear_count(pcnt);
    if (err != BLUSYS_OK) {
        printf("%s clear error: %s\n", label, blusys_err_string(err));
        return false;
    }

    err = blusys_pcnt_start(pcnt);
    if (err != BLUSYS_OK) {
        printf("%s start error: %s\n", label, blusys_err_string(err));
        return false;
    }

    while (count < limit) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        err = blusys_pcnt_get_count(pcnt, &count);
        if (err != BLUSYS_OK) {
            printf("%s get count error: %s\n", label, blusys_err_string(err));
            break;
        }

        printf("%s count=%d\n", label, count);
        blusys_pcnt_basic_drain_watch_queue(ctx);
    }

    err = blusys_pcnt_stop(pcnt);
    if (err != BLUSYS_OK) {
        printf("%s stop error: %s\n", label, blusys_err_string(err));
        return false;
    }

    blusys_pcnt_basic_drain_watch_queue(ctx);
    printf("%s result: %s\n", label, (count >= limit) ? "ok" : "incomplete");
    return count >= limit;
}

void app_main(void)
{
    blusys_pcnt_basic_ctx_t ctx = {
        .queue = NULL,
    };
    static const int watch_points[] = {100, 500};
    blusys_pwm_t *pwm = NULL;
    blusys_pcnt_t *pcnt = NULL;
    blusys_err_t err;
    bool phase_a_ok;
    bool phase_b_ok;
    size_t index;

    printf("Blusys pcnt basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pwm_pin: %d\n", BLUSYS_PCNT_BASIC_PWM_PIN);
    printf("input_pin: %d\n", BLUSYS_PCNT_BASIC_INPUT_PIN);
    printf("jumper the PWM output pin to the PCNT input pin\n");

    ctx.queue = xQueueCreate(8, sizeof(int));
    if (ctx.queue == NULL) {
        printf("queue create error\n");
        return;
    }

    err = blusys_pwm_open(BLUSYS_PCNT_BASIC_PWM_PIN,
                          BLUSYS_PCNT_BASIC_FREQ_HZ,
                          BLUSYS_PCNT_BASIC_DUTY_PERMILLE,
                          &pwm);
    if (err != BLUSYS_OK) {
        printf("pwm open error: %s\n", blusys_err_string(err));
        goto cleanup;
    }

    err = blusys_pcnt_open(BLUSYS_PCNT_BASIC_INPUT_PIN,
                           BLUSYS_PCNT_EDGE_RISING,
                           BLUSYS_PCNT_BASIC_GLITCH_NS,
                           &pcnt);
    if (err != BLUSYS_OK) {
        printf("pcnt open error: %s\n", blusys_err_string(err));
        goto cleanup;
    }

    err = blusys_pcnt_set_callback(pcnt, blusys_pcnt_basic_on_watch, &ctx);
    if (err != BLUSYS_OK) {
        printf("set callback error: %s\n", blusys_err_string(err));
        goto cleanup;
    }

    for (index = 0u; index < (sizeof(watch_points) / sizeof(watch_points[0])); ++index) {
        err = blusys_pcnt_add_watch_point(pcnt, watch_points[index]);
        if (err != BLUSYS_OK) {
            printf("add watch point %d error: %s\n",
                   watch_points[index],
                   blusys_err_string(err));
            goto cleanup;
        }
    }

    err = blusys_pwm_start(pwm);
    if (err != BLUSYS_OK) {
        printf("pwm start error: %s\n", blusys_err_string(err));
        goto cleanup;
    }

    phase_a_ok = blusys_pcnt_basic_run_phase(pcnt, &ctx, "phase_a", BLUSYS_PCNT_BASIC_PHASE_A_LIMIT);
    phase_b_ok = blusys_pcnt_basic_run_phase(pcnt, &ctx, "phase_b", BLUSYS_PCNT_BASIC_PHASE_B_LIMIT);

    printf("watch_callback_result: %s\n", (phase_a_ok && phase_b_ok) ? "ok" : "incomplete");

cleanup:
    if (pcnt != NULL) {
        if (blusys_pcnt_stop(pcnt) != BLUSYS_OK) {
            /* Best-effort cleanup after the demo phases. */
        }
        if (blusys_pcnt_close(pcnt) != BLUSYS_OK) {
            /* Best-effort cleanup after the demo phases. */
        }
    }

    if (pwm != NULL) {
        if (blusys_pwm_stop(pwm) != BLUSYS_OK) {
            /* Best-effort cleanup after the demo phases. */
        }
        if (blusys_pwm_close(pwm) != BLUSYS_OK) {
            /* Best-effort cleanup after the demo phases. */
        }
    }

    if (ctx.queue != NULL) {
        vQueueDelete(ctx.queue);
    }
}
