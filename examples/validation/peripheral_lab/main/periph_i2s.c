#if CONFIG_PERIPHERAL_LAB_SCENARIO_I2S
#include <stdio.h>

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#if CONFIG_BLUSYS_I2S_BASIC_MODE_TX

#define BLUSYS_I2S_BASIC_PORT CONFIG_BLUSYS_I2S_BASIC_PORT
#define BLUSYS_I2S_BASIC_BCLK_PIN CONFIG_BLUSYS_I2S_BASIC_BCLK_PIN
#define BLUSYS_I2S_BASIC_WS_PIN CONFIG_BLUSYS_I2S_BASIC_WS_PIN
#define BLUSYS_I2S_BASIC_DOUT_PIN CONFIG_BLUSYS_I2S_BASIC_DOUT_PIN
#define BLUSYS_I2S_BASIC_MCLK_PIN CONFIG_BLUSYS_I2S_BASIC_MCLK_PIN
#define BLUSYS_I2S_BASIC_SAMPLE_RATE_HZ CONFIG_BLUSYS_I2S_BASIC_SAMPLE_RATE_HZ

#define BLUSYS_I2S_BASIC_BUFFER_FRAMES 256u
#define BLUSYS_I2S_BASIC_TONE_HZ 440u
#define BLUSYS_I2S_BASIC_AMPLITUDE 12000

static void blusys_i2s_basic_fill_buffer(int16_t *samples, size_t frame_count, uint32_t *phase)
{
    const uint32_t half_period_frames = BLUSYS_I2S_BASIC_SAMPLE_RATE_HZ / (BLUSYS_I2S_BASIC_TONE_HZ * 2u);
    size_t frame_index;

    for (frame_index = 0u; frame_index < frame_count; ++frame_index) {
        int16_t sample = ((*phase / half_period_frames) % 2u == 0u) ? BLUSYS_I2S_BASIC_AMPLITUDE : -BLUSYS_I2S_BASIC_AMPLITUDE;

        samples[frame_index * 2u] = sample;
        samples[(frame_index * 2u) + 1u] = sample;
        *phase += 1u;
    }
}

static void run_i2s_tx(void)
{
    blusys_i2s_tx_t *i2s = NULL;
    blusys_err_t err;
    uint32_t phase = 0u;
    static int16_t audio_buffer[BLUSYS_I2S_BASIC_BUFFER_FRAMES * 2u];

    printf("Blusys i2s basic (TX)\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_I2S_BASIC_PORT);
    printf("bclk_pin: %d\n", BLUSYS_I2S_BASIC_BCLK_PIN);
    printf("ws_pin: %d\n", BLUSYS_I2S_BASIC_WS_PIN);
    printf("dout_pin: %d\n", BLUSYS_I2S_BASIC_DOUT_PIN);
    printf("mclk_pin: %d\n", BLUSYS_I2S_BASIC_MCLK_PIN);
    printf("sample_rate_hz: %d\n", BLUSYS_I2S_BASIC_SAMPLE_RATE_HZ);
    printf("connect an external I2S DAC or codec to hear the generated tone\n");

    err = blusys_i2s_tx_open(&(blusys_i2s_tx_config_t) {
                                 .port = BLUSYS_I2S_BASIC_PORT,
                                 .bclk_pin = BLUSYS_I2S_BASIC_BCLK_PIN,
                                 .ws_pin = BLUSYS_I2S_BASIC_WS_PIN,
                                 .dout_pin = BLUSYS_I2S_BASIC_DOUT_PIN,
                                 .mclk_pin = BLUSYS_I2S_BASIC_MCLK_PIN,
                                 .sample_rate_hz = BLUSYS_I2S_BASIC_SAMPLE_RATE_HZ,
                             },
                             &i2s);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_i2s_tx_start(i2s);
    if (err != BLUSYS_OK) {
        printf("start error: %s\n", blusys_err_string(err));
        blusys_i2s_tx_close(i2s);
        return;
    }

    while (true) {
        blusys_i2s_basic_fill_buffer(audio_buffer, BLUSYS_I2S_BASIC_BUFFER_FRAMES, &phase);
        err = blusys_i2s_tx_write(i2s, audio_buffer, sizeof(audio_buffer), BLUSYS_TIMEOUT_FOREVER);
        if (err != BLUSYS_OK) {
            printf("write error: %s\n", blusys_err_string(err));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    blusys_i2s_tx_stop(i2s);
    blusys_i2s_tx_close(i2s);
}

#elif CONFIG_BLUSYS_I2S_BASIC_MODE_RX

#define BLUSYS_I2S_RX_BASIC_BCLK_PIN       CONFIG_BLUSYS_I2S_RX_BASIC_BCLK_PIN
#define BLUSYS_I2S_RX_BASIC_WS_PIN         CONFIG_BLUSYS_I2S_RX_BASIC_WS_PIN
#define BLUSYS_I2S_RX_BASIC_DIN_PIN        CONFIG_BLUSYS_I2S_RX_BASIC_DIN_PIN
#define BLUSYS_I2S_RX_BASIC_SAMPLE_RATE_HZ CONFIG_BLUSYS_I2S_RX_BASIC_SAMPLE_RATE_HZ

#define BLUSYS_I2S_RX_BASIC_BUF_SAMPLES    256u
#define BLUSYS_I2S_RX_BASIC_BUF_BYTES      (BLUSYS_I2S_RX_BASIC_BUF_SAMPLES * 2u * sizeof(int16_t))
#define BLUSYS_I2S_RX_BASIC_READ_COUNT     8u
#define BLUSYS_I2S_RX_BASIC_TIMEOUT_MS     1000

static void run_i2s_rx(void)
{
    blusys_i2s_rx_t *i2s = NULL;
    blusys_err_t err;
    static int16_t buf[BLUSYS_I2S_RX_BASIC_BUF_SAMPLES * 2u];
    size_t bytes_read;
    unsigned int i;

    printf("Blusys i2s basic (RX)\n");
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

#endif

void run_periph_i2s(void)
{
#if CONFIG_BLUSYS_I2S_BASIC_MODE_TX
    run_i2s_tx();
#elif CONFIG_BLUSYS_I2S_BASIC_MODE_RX
    run_i2s_rx();
#endif
}

#else /* !CONFIG_PERIPHERAL_LAB_SCENARIO_I2S */
void run_periph_i2s(void) {}
#endif /* CONFIG_PERIPHERAL_LAB_SCENARIO_I2S */
