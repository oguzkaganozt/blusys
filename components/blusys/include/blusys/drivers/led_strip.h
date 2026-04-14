#ifndef BLUSYS_LED_STRIP_H
#define BLUSYS_LED_STRIP_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_led_strip blusys_led_strip_t;

typedef enum {
    BLUSYS_LED_STRIP_WS2812B = 0,   /* 3-byte GRB, default */
    BLUSYS_LED_STRIP_SK6812_RGB,     /* 3-byte GRB */
    BLUSYS_LED_STRIP_SK6812_RGBW,    /* 4-byte GRBW */
    BLUSYS_LED_STRIP_WS2811,         /* 3-byte RGB */
    BLUSYS_LED_STRIP_APA106,         /* 3-byte RGB */
} blusys_led_strip_type_t;

typedef struct {
    int                     pin;        /* GPIO pin connected to DIN of the LED strip */
    uint32_t                led_count;  /* total number of LEDs in the strip */
    blusys_led_strip_type_t type;       /* LED chip type (default: WS2812B) */
} blusys_led_strip_config_t;

blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip);
blusys_err_t blusys_led_strip_close(blusys_led_strip_t *strip);
blusys_err_t blusys_led_strip_set_pixel(blusys_led_strip_t *strip,
                                         uint32_t index,
                                         uint8_t r, uint8_t g, uint8_t b);
blusys_err_t blusys_led_strip_set_pixel_rgbw(blusys_led_strip_t *strip,
                                              uint32_t index,
                                              uint8_t r, uint8_t g, uint8_t b,
                                              uint8_t w);
blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms);
blusys_err_t blusys_led_strip_clear(blusys_led_strip_t *strip, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_LED_STRIP_H */
