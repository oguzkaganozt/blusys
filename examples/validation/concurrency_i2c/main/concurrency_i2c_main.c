#include <stdio.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

/*
 * Concurrency test: two tasks call blusys_i2c_master_probe() on the same
 * handle simultaneously for multiple iterations.
 *
 * No specific device is required. BLUSYS_OK (device found) and
 * BLUSYS_ERR_IO (no ACK, device absent) are both valid outcomes.
 * Any other error code or a crash is a failure.
 *
 * Pass criteria: zero unexpected errors across all iterations of both tasks.
 */

#define BLUSYS_CONCURRENCY_I2C_PORT    CONFIG_BLUSYS_CONCURRENCY_I2C_PORT
#define BLUSYS_CONCURRENCY_I2C_SDA_PIN CONFIG_BLUSYS_CONCURRENCY_I2C_SDA_PIN
#define BLUSYS_CONCURRENCY_I2C_SCL_PIN CONFIG_BLUSYS_CONCURRENCY_I2C_SCL_PIN
#define BLUSYS_CONCURRENCY_I2C_FREQ_HZ 100000u
#define BLUSYS_CONCURRENCY_I2C_TIMEOUT_MS 20
#define BLUSYS_CONCURRENCY_I2C_PROBE_ADDR 0x3cu
#define BLUSYS_CONCURRENCY_I2C_ITERS 50u

typedef struct {
    blusys_i2c_master_t *i2c;
    uint32_t task_id;
    uint32_t errors;
    TaskHandle_t report_task;
    uint32_t done_bit;
} blusys_concurrency_i2c_ctx_t;

static bool blusys_concurrency_i2c_is_valid_probe_result(blusys_err_t err)
{
    return (err == BLUSYS_OK) || (err == BLUSYS_ERR_IO) || (err == BLUSYS_ERR_TIMEOUT);
}

static void blusys_concurrency_i2c_worker(void *arg)
{
    blusys_concurrency_i2c_ctx_t *ctx = arg;
    uint32_t i;
    blusys_err_t err;

    for (i = 0u; i < BLUSYS_CONCURRENCY_I2C_ITERS; ++i) {
        err = blusys_i2c_master_probe(ctx->i2c,
                                      BLUSYS_CONCURRENCY_I2C_PROBE_ADDR,
                                      BLUSYS_CONCURRENCY_I2C_TIMEOUT_MS);
        if (!blusys_concurrency_i2c_is_valid_probe_result(err)) {
            printf("task_%u iter_%u unexpected_error: %s\n",
                   (unsigned int) ctx->task_id,
                   (unsigned int) i,
                   blusys_err_string(err));
            ctx->errors += 1u;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

void app_main(void)
{
    blusys_concurrency_i2c_ctx_t ctx_a = {
        .i2c = NULL,
        .task_id = 0u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = (1u << 0),
    };
    blusys_concurrency_i2c_ctx_t ctx_b = {
        .i2c = NULL,
        .task_id = 1u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = (1u << 1),
    };
    blusys_i2c_master_t *i2c = NULL;
    blusys_err_t err;
    uint32_t notified_bits = 0u;
    uint32_t received_bits = 0u;
    uint32_t done_mask = ctx_a.done_bit | ctx_b.done_bit;

    printf("Blusys concurrency i2c\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_CONCURRENCY_I2C_PORT);
    printf("sda_pin: %d\n", BLUSYS_CONCURRENCY_I2C_SDA_PIN);
    printf("scl_pin: %d\n", BLUSYS_CONCURRENCY_I2C_SCL_PIN);
    printf("probe_addr: 0x%02x\n", (unsigned int) BLUSYS_CONCURRENCY_I2C_PROBE_ADDR);
    printf("tasks: 2  iterations_per_task: %u\n", (unsigned int) BLUSYS_CONCURRENCY_I2C_ITERS);

    err = blusys_i2c_master_open(BLUSYS_CONCURRENCY_I2C_PORT,
                                 BLUSYS_CONCURRENCY_I2C_SDA_PIN,
                                 BLUSYS_CONCURRENCY_I2C_SCL_PIN,
                                 BLUSYS_CONCURRENCY_I2C_FREQ_HZ,
                                 &i2c);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    ctx_a.i2c = i2c;
    ctx_b.i2c = i2c;
    ctx_a.report_task = xTaskGetCurrentTaskHandle();
    ctx_b.report_task = xTaskGetCurrentTaskHandle();

    xTaskCreate(blusys_concurrency_i2c_worker, "i2c_worker_a", 4096, &ctx_a, 5, NULL);
    xTaskCreate(blusys_concurrency_i2c_worker, "i2c_worker_b", 4096, &ctx_b, 5, NULL);

    while ((notified_bits & done_mask) != done_mask) {
        xTaskNotifyWait(0u, UINT32_MAX, &received_bits, portMAX_DELAY);
        notified_bits |= received_bits;
    }

    printf("task_a errors: %u\n", (unsigned int) ctx_a.errors);
    printf("task_b errors: %u\n", (unsigned int) ctx_b.errors);
    printf("concurrent_result: %s\n",
           ((ctx_a.errors == 0u) && (ctx_b.errors == 0u)) ? "ok" : "fail");

    err = blusys_i2c_master_close(i2c);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
