#include <stdio.h>

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

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

void app_main(void)
{
    blusys_i2s_tx_t *i2s = NULL;
    blusys_err_t err;
    uint32_t phase = 0u;
    static int16_t audio_buffer[BLUSYS_I2S_BASIC_BUFFER_FRAMES * 2u];

    printf("Blusys i2s basic\n");
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
