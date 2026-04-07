#ifndef BLUSYS_ENCODER_H
#define BLUSYS_ENCODER_H

#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_encoder blusys_encoder_t;

typedef enum {
    BLUSYS_ENCODER_EVENT_CW,
    BLUSYS_ENCODER_EVENT_CCW,
    BLUSYS_ENCODER_EVENT_PRESS,
    BLUSYS_ENCODER_EVENT_RELEASE,
    BLUSYS_ENCODER_EVENT_LONG_PRESS,
} blusys_encoder_event_t;

typedef void (*blusys_encoder_callback_t)(blusys_encoder_t *encoder,
                                          blusys_encoder_event_t event,
                                          int position,
                                          void *user_ctx);

typedef struct {
    int      clk_pin;
    int      dt_pin;
    int      sw_pin;           /* -1 to disable button */
    uint32_t glitch_filter_ns; /* PCNT glitch filter (ESP32/S3 only), 0 = disabled */
    uint32_t debounce_ms;      /* Button debounce, 0 = default 50 ms */
    uint32_t long_press_ms;    /* Button long press, 0 = disabled */
    int      steps_per_detent; /* Pulses per mechanical detent, 0 = default 4 */
} blusys_encoder_config_t;

blusys_err_t blusys_encoder_open(const blusys_encoder_config_t *config,
                                  blusys_encoder_t **out_encoder);
blusys_err_t blusys_encoder_close(blusys_encoder_t *encoder);
blusys_err_t blusys_encoder_set_callback(blusys_encoder_t *encoder,
                                          blusys_encoder_callback_t callback,
                                          void *user_ctx);
blusys_err_t blusys_encoder_get_position(blusys_encoder_t *encoder,
                                          int *out_position);
blusys_err_t blusys_encoder_reset_position(blusys_encoder_t *encoder);

#ifdef __cplusplus
}
#endif

#endif
