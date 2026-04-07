#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "blusys/blusys.h"
#include "blusys/encoder.h"

static const char *event_name(blusys_encoder_event_t event)
{
    switch (event) {
    case BLUSYS_ENCODER_EVENT_CW:         return "CW";
    case BLUSYS_ENCODER_EVENT_CCW:        return "CCW";
    case BLUSYS_ENCODER_EVENT_PRESS:      return "PRESS";
    case BLUSYS_ENCODER_EVENT_RELEASE:    return "RELEASE";
    case BLUSYS_ENCODER_EVENT_LONG_PRESS: return "LONG_PRESS";
    default:                              return "UNKNOWN";
    }
}

static void on_encoder_event(blusys_encoder_t *enc, blusys_encoder_event_t event,
                              int position, void *ctx)
{
    (void)enc;
    (void)ctx;
    printf("encoder: %-10s  position=%d\n", event_name(event), position);
}

void app_main(void)
{
    blusys_encoder_t *enc = NULL;
    blusys_err_t err;

    printf("Blusys encoder basic example\n");
    printf("target: %s\n", blusys_target_name());

    const blusys_encoder_config_t cfg = {
        .clk_pin          = CONFIG_BLUSYS_ENCODER_BASIC_CLK_PIN,
        .dt_pin           = CONFIG_BLUSYS_ENCODER_BASIC_DT_PIN,
        .sw_pin           = CONFIG_BLUSYS_ENCODER_BASIC_SW_PIN,
        .glitch_filter_ns = 1000,
        .debounce_ms      = 50,
        .long_press_ms    = 1000,
    };

    err = blusys_encoder_open(&cfg, &enc);
    if (err != BLUSYS_OK) {
        printf("encoder open failed: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_encoder_set_callback(enc, on_encoder_event, NULL);
    if (err != BLUSYS_OK) {
        printf("set callback failed: %s\n", blusys_err_string(err));
        blusys_encoder_close(enc);
        return;
    }

    printf("waiting for encoder events ...\n");

    while (true) {
        int pos;
        blusys_encoder_get_position(enc, &pos);
        printf("position: %d\n", pos);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
