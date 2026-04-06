#ifndef BLUSYS_LED_STRIP_H
#define BLUSYS_LED_STRIP_H

#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_led_strip blusys_led_strip_t;

typedef struct {
    int      pin;        /* GPIO pin connected to DIN of the LED strip */
    uint32_t led_count;  /* total number of LEDs in the strip */
} blusys_led_strip_config_t;

blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip);
blusys_err_t blusys_led_strip_close(blusys_led_strip_t *strip);
blusys_err_t blusys_led_strip_set_pixel(blusys_led_strip_t *strip,
                                         uint32_t index,
                                         uint8_t r, uint8_t g, uint8_t b);
blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms);
blusys_err_t blusys_led_strip_clear(blusys_led_strip_t *strip, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_LED_STRIP_H */
