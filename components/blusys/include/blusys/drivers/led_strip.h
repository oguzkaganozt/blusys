/**
 * @file led_strip.h
 * @brief Addressable LED strip driver (WS2812B / SK6812 / WS2811 / APA106).
 *
 * Pixel writes are buffered in RAM; `refresh` flushes the frame over RMT.
 * Only WS-family chips are supported — the family is chosen via
 * ::blusys_led_strip_type_t. See docs/hal/led_strip.md.
 */

#ifndef BLUSYS_LED_STRIP_H
#define BLUSYS_LED_STRIP_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open LED strip. */
typedef struct blusys_led_strip blusys_led_strip_t;

/** @brief LED chip family. */
typedef enum {
    BLUSYS_LED_STRIP_WS2812B = 0,    /**< WS2812B, 3-byte GRB (default). */
    BLUSYS_LED_STRIP_SK6812_RGB,     /**< SK6812 RGB, 3-byte GRB. */
    BLUSYS_LED_STRIP_SK6812_RGBW,    /**< SK6812 RGBW, 4-byte GRBW. */
    BLUSYS_LED_STRIP_WS2811,         /**< WS2811, 3-byte RGB. */
    BLUSYS_LED_STRIP_APA106,         /**< APA106, 3-byte RGB. */
} blusys_led_strip_type_t;

/** @brief Configuration for ::blusys_led_strip_open. */
typedef struct {
    int                     pin;        /**< GPIO pin connected to DIN of the strip. */
    uint32_t                led_count;  /**< Total number of LEDs. */
    blusys_led_strip_type_t type;       /**< LED chip family (default `WS2812B`). */
} blusys_led_strip_config_t;

/**
 * @brief Open an LED strip, allocate the pixel buffer, and prepare the RMT channel.
 * @param config     Configuration.
 * @param out_strip  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`.
 */
blusys_err_t blusys_led_strip_open(const blusys_led_strip_config_t *config,
                                    blusys_led_strip_t **out_strip);

/**
 * @brief Release the strip, free buffers, and tear down the RMT channel.
 * @param strip  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p strip is NULL.
 */
blusys_err_t blusys_led_strip_close(blusys_led_strip_t *strip);

/**
 * @brief Stage one pixel's RGB values. Call ::blusys_led_strip_refresh to flush.
 * @param strip  Handle.
 * @param index  LED index (`0` = first LED).
 * @param r      Red channel (0–255).
 * @param g      Green channel (0–255).
 * @param b      Blue channel (0–255).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p index is out of range.
 */
blusys_err_t blusys_led_strip_set_pixel(blusys_led_strip_t *strip,
                                         uint32_t index,
                                         uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Stage one pixel's RGBW values (requires the `SK6812_RGBW` family).
 * @param strip  Handle.
 * @param index  LED index.
 * @param r      Red channel.
 * @param g      Green channel.
 * @param b      Blue channel.
 * @param w      White channel.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_NOT_SUPPORTED` when the strip is not RGBW.
 */
blusys_err_t blusys_led_strip_set_pixel_rgbw(blusys_led_strip_t *strip,
                                              uint32_t index,
                                              uint8_t r, uint8_t g, uint8_t b,
                                              uint8_t w);

/**
 * @brief Transmit the staged buffer to the strip.
 * @param strip       Handle.
 * @param timeout_ms  Transmission timeout in ms; use `BLUSYS_TIMEOUT_FOREVER` to block.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, or `BLUSYS_ERR_IO` on RMT failure.
 */
blusys_err_t blusys_led_strip_refresh(blusys_led_strip_t *strip, int timeout_ms);

/**
 * @brief Zero the pixel buffer and refresh, turning every LED off.
 * @param strip       Handle.
 * @param timeout_ms  Transmission timeout in ms.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, or `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_led_strip_clear(blusys_led_strip_t *strip, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
