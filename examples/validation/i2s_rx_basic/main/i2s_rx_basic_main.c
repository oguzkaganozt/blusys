#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_I2S_RX_BASIC_BCLK_PIN       CONFIG_BLUSYS_I2S_RX_BASIC_BCLK_PIN
#define BLUSYS_I2S_RX_BASIC_WS_PIN         CONFIG_BLUSYS_I2S_RX_BASIC_WS_PIN
#define BLUSYS_I2S_RX_BASIC_DIN_PIN        CONFIG_BLUSYS_I2S_RX_BASIC_DIN_PIN
#define BLUSYS_I2S_RX_BASIC_SAMPLE_RATE_HZ CONFIG_BLUSYS_I2S_RX_BASIC_SAMPLE_RATE_HZ

#define BLUSYS_I2S_RX_BASIC_BUF_SAMPLES    256u
#define BLUSYS_I2S_RX_BASIC_BUF_BYTES      (BLUSYS_I2S_RX_BASIC_BUF_SAMPLES * 2u * sizeof(int16_t))
#define BLUSYS_I2S_RX_BASIC_READ_COUNT     8u
#define BLUSYS_I2S_RX_BASIC_TIMEOUT_MS     1000

void app_main(void)
{
    blusys_i2s_rx_t *i2s = NULL;
    blusys_err_t err;
    static int16_t buf[BLUSYS_I2S_RX_BASIC_BUF_SAMPLES * 2u];   /* stereo: L+R interleaved */
    size_t bytes_read;
    unsigned int i;

    printf("Blusys i2s rx basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("i2s_rx supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_I2S_RX) ? "yes" : "no");

    if (!blusys_target_supports(BLUSYS_FEATURE_I2S_RX)) {
        printf("i2s rx not supported on this target\n");
        return;
    }

    printf("bclk: %d  ws: %d  din: %d  rate: %u Hz\n",
           BLUSYS_I2S_RX_BASIC_BCLK_PIN, BLUSYS_I2S_RX_BASIC_WS_PIN,
           BLUSYS_I2S_RX_BASIC_DIN_PIN, (unsigned int) BLUSYS_I2S_RX_BASIC_SAMPLE_RATE_HZ);

    blusys_i2s_rx_config_t config = {
        .port           = 0,
        .bclk_pin       = BLUSYS_I2S_RX_BASIC_BCLK_PIN,
        .ws_pin         = BLUSYS_I2S_RX_BASIC_WS_PIN,
        .din_pin        = BLUSYS_I2S_RX_BASIC_DIN_PIN,
        .mclk_pin       = -1,
        .sample_rate_hz = BLUSYS_I2S_RX_BASIC_SAMPLE_RATE_HZ,
    };

    err = blusys_i2s_rx_open(&config, &i2s);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_i2s_rx_start(i2s);
    if (err != BLUSYS_OK) {
        printf("start error: %s\n", blusys_err_string(err));
        blusys_i2s_rx_close(i2s);
        return;
    }

    printf("connect an I2S microphone (e.g. INMP441) to the configured pins\n");

    for (i = 0u; i < BLUSYS_I2S_RX_BASIC_READ_COUNT; ++i) {
        err = blusys_i2s_rx_read(i2s, buf, BLUSYS_I2S_RX_BASIC_BUF_BYTES,
                                 &bytes_read, BLUSYS_I2S_RX_BASIC_TIMEOUT_MS);
        if (err != BLUSYS_OK) {
            printf("read error: %s\n", blusys_err_string(err));
            break;
        }
        printf("read %u: %u bytes  first sample L=%d R=%d\n",
               i, (unsigned int) bytes_read, (int) buf[0], (int) buf[1]);
    }

    blusys_i2s_rx_stop(i2s);
    blusys_i2s_rx_close(i2s);
    printf("done\n");
}
