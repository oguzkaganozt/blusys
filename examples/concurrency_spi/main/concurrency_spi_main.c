#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

/*
 * Concurrency test: two tasks call blusys_spi_transfer() on the same handle
 * simultaneously. With MOSI wired to MISO (loopback), each transfer must
 * return TX == RX. The handle mutex must serialize access cleanly.
 *
 * Pass criteria: zero transfer errors, zero data mismatches across all
 * iterations of both tasks.
 */

#define BLUSYS_CONCURRENCY_SPI_BUS      CONFIG_BLUSYS_CONCURRENCY_SPI_BUS
#define BLUSYS_CONCURRENCY_SPI_SCLK_PIN CONFIG_BLUSYS_CONCURRENCY_SPI_SCLK_PIN
#define BLUSYS_CONCURRENCY_SPI_MOSI_PIN CONFIG_BLUSYS_CONCURRENCY_SPI_MOSI_PIN
#define BLUSYS_CONCURRENCY_SPI_MISO_PIN CONFIG_BLUSYS_CONCURRENCY_SPI_MISO_PIN
#define BLUSYS_CONCURRENCY_SPI_CS_PIN   CONFIG_BLUSYS_CONCURRENCY_SPI_CS_PIN
#define BLUSYS_CONCURRENCY_SPI_FREQ_HZ  1000000u
#define BLUSYS_CONCURRENCY_SPI_ITERS    50u

typedef struct {
    blusys_spi_t *spi;
    uint32_t task_id;
    uint32_t errors;
    TaskHandle_t report_task;
    uint32_t done_bit;
} blusys_concurrency_spi_ctx_t;

static void blusys_concurrency_spi_worker(void *arg)
{
    blusys_concurrency_spi_ctx_t *ctx = arg;
    uint8_t tx_data[8];
    uint8_t rx_data[8];
    uint32_t i;
    uint32_t j;
    blusys_err_t err;

    for (i = 0u; i < BLUSYS_CONCURRENCY_SPI_ITERS; ++i) {
        for (j = 0u; j < sizeof(tx_data); ++j) {
            tx_data[j] = (uint8_t)((ctx->task_id * 0x10u) + i + j);
        }
        memset(rx_data, 0, sizeof(rx_data));

        err = blusys_spi_transfer(ctx->spi, tx_data, rx_data, sizeof(tx_data));
        if (err != BLUSYS_OK) {
            printf("task_%u iter_%u transfer_error: %s\n",
                   (unsigned int) ctx->task_id,
                   (unsigned int) i,
                   blusys_err_string(err));
            ctx->errors += 1u;
        } else if (memcmp(tx_data, rx_data, sizeof(tx_data)) != 0) {
            printf("task_%u iter_%u mismatch\n",
                   (unsigned int) ctx->task_id,
                   (unsigned int) i);
            ctx->errors += 1u;
        }
    }

    xTaskNotify(ctx->report_task, ctx->done_bit, eSetBits);
    vTaskDelete(NULL);
}

void app_main(void)
{
    blusys_concurrency_spi_ctx_t ctx_a = {
        .spi = NULL,
        .task_id = 0u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = (1u << 0),
    };
    blusys_concurrency_spi_ctx_t ctx_b = {
        .spi = NULL,
        .task_id = 1u,
        .errors = 0u,
        .report_task = NULL,
        .done_bit = (1u << 1),
    };
    blusys_spi_t *spi = NULL;
    blusys_err_t err;
    uint32_t notified_bits = 0u;
    uint32_t received_bits = 0u;
    uint32_t done_mask = ctx_a.done_bit | ctx_b.done_bit;

    printf("Blusys concurrency spi\n");
    printf("target: %s\n", blusys_target_name());
    printf("bus: %d\n", BLUSYS_CONCURRENCY_SPI_BUS);
    printf("sclk_pin: %d\n", BLUSYS_CONCURRENCY_SPI_SCLK_PIN);
    printf("mosi_pin: %d\n", BLUSYS_CONCURRENCY_SPI_MOSI_PIN);
    printf("miso_pin: %d\n", BLUSYS_CONCURRENCY_SPI_MISO_PIN);
    printf("cs_pin: %d\n", BLUSYS_CONCURRENCY_SPI_CS_PIN);
    printf("wire mosi_pin to miso_pin before running this test\n");
    printf("tasks: 2  iterations_per_task: %u\n", (unsigned int) BLUSYS_CONCURRENCY_SPI_ITERS);

    err = blusys_spi_open(BLUSYS_CONCURRENCY_SPI_BUS,
                          BLUSYS_CONCURRENCY_SPI_SCLK_PIN,
                          BLUSYS_CONCURRENCY_SPI_MOSI_PIN,
                          BLUSYS_CONCURRENCY_SPI_MISO_PIN,
                          BLUSYS_CONCURRENCY_SPI_CS_PIN,
                          BLUSYS_CONCURRENCY_SPI_FREQ_HZ,
                          &spi);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    ctx_a.spi = spi;
    ctx_b.spi = spi;
    ctx_a.report_task = xTaskGetCurrentTaskHandle();
    ctx_b.report_task = xTaskGetCurrentTaskHandle();

    xTaskCreate(blusys_concurrency_spi_worker, "spi_worker_a", 4096, &ctx_a, 5, NULL);
    xTaskCreate(blusys_concurrency_spi_worker, "spi_worker_b", 4096, &ctx_b, 5, NULL);

    while ((notified_bits & done_mask) != done_mask) {
        xTaskNotifyWait(0u, UINT32_MAX, &received_bits, portMAX_DELAY);
        notified_bits |= received_bits;
    }

    printf("task_a errors: %u\n", (unsigned int) ctx_a.errors);
    printf("task_b errors: %u\n", (unsigned int) ctx_b.errors);
    printf("concurrent_result: %s\n",
           ((ctx_a.errors == 0u) && (ctx_b.errors == 0u)) ? "ok" : "fail");

    err = blusys_spi_close(spi);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
