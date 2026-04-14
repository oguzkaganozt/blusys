#ifndef BLUSYS_SEVEN_SEG_H
#define BLUSYS_SEVEN_SEG_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of digits supported per display instance. */
#define BLUSYS_SEVEN_SEG_MAX_DIGITS 8

/**
 * @brief Driver backend for a 7-segment display.
 *
 * - GPIO:    One GPIO per segment (a–g, dp) plus one common pin per digit.
 *            Software multiplexing via esp_timer.
 * - 74HC595: 8-bit shift register (3 GPIO: DATA/CLK/LATCH) plus one common
 *            pin per digit. Software multiplexing via esp_timer.
 * - MAX7219: SPI-based controller (3 GPIO: MOSI/CLK/CS). Handles multiplexing
 *            internally. Supports up to 8 digits and hardware brightness control.
 */
typedef enum {
    BLUSYS_SEVEN_SEG_DRIVER_GPIO    = 0,
    BLUSYS_SEVEN_SEG_DRIVER_74HC595 = 1,
    BLUSYS_SEVEN_SEG_DRIVER_MAX7219 = 2,
} blusys_seven_seg_driver_t;

/**
 * @brief Configuration for the direct-GPIO driver.
 *
 * Each segment pin drives one segment of the display (shared across all digits).
 * Set @c pin_dp to -1 if no decimal point is wired.
 * Set unused @c common_pins[] slots to -1.
 */
typedef struct {
    int  pin_a;                                   /**< Segment A (top) */
    int  pin_b;                                   /**< Segment B (top-right) */
    int  pin_c;                                   /**< Segment C (bottom-right) */
    int  pin_d;                                   /**< Segment D (bottom) */
    int  pin_e;                                   /**< Segment E (bottom-left) */
    int  pin_f;                                   /**< Segment F (top-left) */
    int  pin_g;                                   /**< Segment G (middle) */
    int  pin_dp;                                  /**< Decimal point (-1 = not wired) */
    int  common_pins[BLUSYS_SEVEN_SEG_MAX_DIGITS];/**< Per-digit common anode/cathode */
    bool common_anode;                            /**< true = common anode, false = common cathode */
} blusys_seven_seg_gpio_config_t;

/**
 * @brief Configuration for the 74HC595 shift-register driver.
 *
 * Three GPIO pins drive the shift register; one common pin per digit handles
 * digit selection. Set unused @c common_pins[] slots to -1.
 */
typedef struct {
    int  data_pin;                                /**< SER — serial data input */
    int  clk_pin;                                 /**< SRCLK — shift clock */
    int  latch_pin;                               /**< RCLK — storage/latch clock */
    int  common_pins[BLUSYS_SEVEN_SEG_MAX_DIGITS];/**< Per-digit common anode/cathode */
    bool common_anode;                            /**< true = common anode, false = common cathode */
} blusys_seven_seg_595_config_t;

/**
 * @brief Configuration for the MAX7219 SPI driver.
 *
 * The MAX7219 handles digit multiplexing internally. It uses SPI2_HOST.
 * Up to 8 digits are supported. Hardware brightness control (0–15) is available
 * via @c blusys_seven_seg_set_brightness().
 */
typedef struct {
    int mosi_pin; /**< MOSI — data to MAX7219 */
    int clk_pin;  /**< CLK  — SPI clock */
    int cs_pin;   /**< CS   — chip select (active low) */
} blusys_seven_seg_max7219_config_t;

/**
 * @brief Top-level configuration passed to @c blusys_seven_seg_open().
 *
 * Set @c driver to select the backend, @c digit_count to the number of digits
 * (1–8), then fill in the matching union member.
 */
typedef struct {
    blusys_seven_seg_driver_t driver;      /**< Which driver backend to use */
    uint8_t                   digit_count; /**< Number of digits (1–8) */
    union {
        blusys_seven_seg_gpio_config_t    gpio;    /**< Direct GPIO config */
        blusys_seven_seg_595_config_t     hc595;   /**< 74HC595 config */
        blusys_seven_seg_max7219_config_t max7219; /**< MAX7219 config */
    };
} blusys_seven_seg_config_t;

typedef struct blusys_seven_seg blusys_seven_seg_t;

/**
 * @brief Open a 7-segment display.
 *
 * Configures GPIO pins, initialises the driver, and (for GPIO/74HC595) starts
 * the multiplexing timer. On MAX7219 the chip is brought out of shutdown mode.
 *
 * @param config     Driver configuration. Must not be NULL.
 * @param out_display Output handle. Must not be NULL.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_seven_seg_open(const blusys_seven_seg_config_t *config,
                                   blusys_seven_seg_t **out_display);

/**
 * @brief Close a 7-segment display and free all resources.
 *
 * Stops the multiplexing timer (GPIO/74HC595), blanks all digits, resets GPIO
 * pins, and frees the handle.
 *
 * @param display Handle returned by @c blusys_seven_seg_open(). Must not be NULL.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_seven_seg_close(blusys_seven_seg_t *display);

/**
 * @brief Display a signed integer.
 *
 * The value is rendered right-aligned. If @c leading_zeros is true, unused
 * high-order digits are filled with '0'; otherwise they are blank.
 * A leading minus sign is shown for negative values using the leftmost digit.
 * Returns @c BLUSYS_ERR_INVALID_ARG if the value does not fit.
 *
 * @param display       Display handle.
 * @param value         Value to display.
 * @param leading_zeros Fill high-order digits with '0' instead of blank.
 * @return BLUSYS_OK or BLUSYS_ERR_INVALID_ARG.
 */
blusys_err_t blusys_seven_seg_write_int(blusys_seven_seg_t *display,
                                        int value, bool leading_zeros);

/**
 * @brief Display a hexadecimal value (0–F digits).
 *
 * The value is rendered right-aligned with leading zeros.
 * Returns @c BLUSYS_ERR_INVALID_ARG if the value does not fit in @c digit_count hex digits.
 *
 * @param display Display handle.
 * @param value   Value to display in hex.
 * @return BLUSYS_OK or BLUSYS_ERR_INVALID_ARG.
 */
blusys_err_t blusys_seven_seg_write_hex(blusys_seven_seg_t *display, uint32_t value);

/**
 * @brief Write raw segment bitmasks directly to the display buffer.
 *
 * Each byte in @p segments encodes one digit: bit0=a, bit1=b, …, bit6=g, bit7=dp.
 * Index 0 maps to the leftmost digit.
 *
 * @param display  Display handle.
 * @param segments Array of segment bitmasks.
 * @param count    Number of elements; must be ≤ @c digit_count.
 * @return BLUSYS_OK or BLUSYS_ERR_INVALID_ARG.
 */
blusys_err_t blusys_seven_seg_write_raw(blusys_seven_seg_t *display,
                                        const uint8_t *segments, uint8_t count);

/**
 * @brief Set or clear decimal point segments.
 *
 * @p digit_mask is a bitmask where bit N controls the decimal point of digit N
 * (bit 0 = leftmost). Existing segment data is preserved; only the dp bit is
 * modified.
 *
 * @param display    Display handle.
 * @param digit_mask Bitmask of digits whose decimal point should be lit.
 * @return BLUSYS_OK or BLUSYS_ERR_INVALID_ARG.
 */
blusys_err_t blusys_seven_seg_set_dp(blusys_seven_seg_t *display, uint8_t digit_mask);

/**
 * @brief Set display brightness.
 *
 * Level 0 is dimmest, 15 is brightest. Only supported on the MAX7219 driver;
 * GPIO and 74HC595 drivers return @c BLUSYS_ERR_NOT_SUPPORTED.
 *
 * @param display Display handle.
 * @param level   Brightness level (0–15).
 * @return BLUSYS_OK, BLUSYS_ERR_NOT_SUPPORTED, or BLUSYS_ERR_INVALID_ARG.
 */
blusys_err_t blusys_seven_seg_set_brightness(blusys_seven_seg_t *display, uint8_t level);

/**
 * @brief Blank all digits.
 *
 * @param display Display handle.
 * @return BLUSYS_OK or BLUSYS_ERR_INVALID_ARG.
 */
blusys_err_t blusys_seven_seg_clear(blusys_seven_seg_t *display);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_SEVEN_SEG_H */
