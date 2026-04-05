#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_RMT_RX_BASIC_PIN          CONFIG_BLUSYS_RMT_RX_BASIC_PIN
#define BLUSYS_RMT_RX_BASIC_MIN_NS       1250u      /* ignore pulses shorter than 1.25 µs */
#define BLUSYS_RMT_RX_BASIC_MAX_NS       12000000u  /* end frame after 12 ms of silence */
#define BLUSYS_RMT_RX_BASIC_MAX_PULSES   256u
#define BLUSYS_RMT_RX_BASIC_TIMEOUT_MS   30000      /* wait up to 30 s for a frame */
#define BLUSYS_RMT_RX_BASIC_FRAMES       3u

void app_main(void)
{
    blusys_rmt_rx_t *rmt_rx = NULL;
    blusys_err_t err;
    static blusys_rmt_pulse_t pulses[BLUSYS_RMT_RX_BASIC_MAX_PULSES];
    size_t count;
    size_t i;
    unsigned int frame;

    printf("Blusys rmt rx basic\n");
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
