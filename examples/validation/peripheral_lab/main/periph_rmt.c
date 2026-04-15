#if CONFIG_PERIPHERAL_LAB_SCENARIO_RMT
#include <stdio.h>

#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#if CONFIG_BLUSYS_RMT_BASIC_MODE_TX

#define BLUSYS_RMT_BASIC_PIN CONFIG_BLUSYS_RMT_BASIC_PIN

static const blusys_rmt_pulse_t blusys_rmt_basic_waveform[] = {
    {.level = true, .duration_us = 1000u},
    {.level = false, .duration_us = 1000u},
    {.level = true, .duration_us = 500u},
    {.level = false, .duration_us = 500u},
    {.level = true, .duration_us = 250u},
    {.level = false, .duration_us = 250u},
};

static void run_rmt_tx(void)
{
    blusys_rmt_t *rmt = NULL;
    blusys_err_t err;

    printf("Blusys rmt basic (TX)\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d\n", BLUSYS_RMT_BASIC_PIN);
    printf("set a board-safe output pin in menuconfig if this default does not match your board\n");

    err = blusys_rmt_open(BLUSYS_RMT_BASIC_PIN, false, &rmt);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    while (true) {
        err = blusys_rmt_write(rmt,
                               blusys_rmt_basic_waveform,
                               sizeof(blusys_rmt_basic_waveform) / sizeof(blusys_rmt_basic_waveform[0]),
                               BLUSYS_TIMEOUT_FOREVER);
        if (err != BLUSYS_OK) {
            printf("write error: %s\n", blusys_err_string(err));
            break;
        }

        printf("sent %u pulses\n",
               (unsigned int) (sizeof(blusys_rmt_basic_waveform) / sizeof(blusys_rmt_basic_waveform[0])));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    blusys_rmt_close(rmt);
}

#elif CONFIG_BLUSYS_RMT_BASIC_MODE_RX

#define BLUSYS_RMT_RX_BASIC_PIN          CONFIG_BLUSYS_RMT_RX_BASIC_PIN
#define BLUSYS_RMT_RX_BASIC_MIN_NS       1250u
#define BLUSYS_RMT_RX_BASIC_MAX_NS       12000000u
#define BLUSYS_RMT_RX_BASIC_MAX_PULSES   256u
#define BLUSYS_RMT_RX_BASIC_TIMEOUT_MS   30000
#define BLUSYS_RMT_RX_BASIC_FRAMES       3u

static void run_rmt_rx(void)
{
    blusys_rmt_rx_t *rmt_rx = NULL;
    blusys_err_t err;
    static blusys_rmt_pulse_t pulses[BLUSYS_RMT_RX_BASIC_MAX_PULSES];
    size_t count;
    size_t i;
    unsigned int frame;

    printf("Blusys rmt basic (RX)\n");
    printf("target: %s\n", blusys_target_name());
    printf("rmt_rx supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_RMT_RX) ? "yes" : "no");

    if (!blusys_target_supports(BLUSYS_FEATURE_RMT_RX)) {
        printf("rmt rx not supported on this target\n");
        return;
    }

    printf("pin: %d\n", BLUSYS_RMT_RX_BASIC_PIN);

    blusys_rmt_rx_config_t config = {
        .signal_range_min_ns = BLUSYS_RMT_RX_BASIC_MIN_NS,
        .signal_range_max_ns = BLUSYS_RMT_RX_BASIC_MAX_NS,
    };

    err = blusys_rmt_rx_open(BLUSYS_RMT_RX_BASIC_PIN, &config, &rmt_rx);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("connect an IR receiver (e.g. VS1838B) and point a remote at it\n");

    for (frame = 0u; frame < BLUSYS_RMT_RX_BASIC_FRAMES; ++frame) {
        printf("waiting for frame %u...\n", frame);

        err = blusys_rmt_rx_read(rmt_rx, pulses, BLUSYS_RMT_RX_BASIC_MAX_PULSES,
                                 &count, BLUSYS_RMT_RX_BASIC_TIMEOUT_MS);
        if (err == BLUSYS_ERR_TIMEOUT) {
            printf("timeout — no signal received\n");
            continue;
        }
        if (err != BLUSYS_OK) {
            printf("read error: %s\n", blusys_err_string(err));
            break;
        }

        printf("frame %u: %u pulses\n", frame, (unsigned int) count);
        for (i = 0u; (i < count) && (i < 16u); ++i) {
            printf("  [%u] level=%u duration=%u us\n",
                   (unsigned int) i,
                   (unsigned int) pulses[i].level,
                   (unsigned int) pulses[i].duration_us);
        }
        if (count > 16u) {
            printf("  ... (%u more)\n", (unsigned int) (count - 16u));
        }
    }

    blusys_rmt_rx_close(rmt_rx);
    printf("done\n");
}

#endif

void run_periph_rmt(void)
{
#if CONFIG_BLUSYS_RMT_BASIC_MODE_TX
    run_rmt_tx();
#elif CONFIG_BLUSYS_RMT_BASIC_MODE_RX
    run_rmt_rx();
#endif
}

#else /* !CONFIG_PERIPHERAL_LAB_SCENARIO_RMT */
void run_periph_rmt(void) {}
#endif /* CONFIG_PERIPHERAL_LAB_SCENARIO_RMT */
