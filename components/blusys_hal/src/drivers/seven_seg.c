#include "blusys/drivers/seven_seg.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/gpio.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"

/* ---------------------------------------------------------------------------
 * Segment encoding
 *
 * Bit layout (shared by internal buffer, GPIO, and 74HC595 output):
 *   bit 0 = a (top)
 *   bit 1 = b (top-right)
 *   bit 2 = c (bottom-right)
 *   bit 3 = d (bottom)
 *   bit 4 = e (bottom-left)
 *   bit 5 = f (top-left)
 *   bit 6 = g (middle)
 *   bit 7 = dp (decimal point)
 * --------------------------------------------------------------------------- */

static const uint8_t s_segments[16] = {
    0x3F, /* 0: abcdef   */
    0x06, /* 1: bc       */
    0x5B, /* 2: abdeg    */
    0x4F, /* 3: abcdg    */
    0x66, /* 4: bcfg     */
    0x6D, /* 5: acdfg    */
    0x7D, /* 6: acdefg   */
    0x07, /* 7: abc      */
    0x7F, /* 8: abcdefg  */
    0x6F, /* 9: abcdfg   */
    0x77, /* A: abcefg   */
    0x7C, /* b: cdefg    */
    0x39, /* C: adef     */
    0x5E, /* d: bcdeg    */
    0x79, /* E: adefg    */
    0x71, /* F: aefg     */
};

#define SEG_BLANK 0x00u
#define SEG_MINUS 0x40u /* g only */

/* Multiplexing period: 2 ms per digit → ≥62 Hz refresh at 8 digits */
#define MUX_PERIOD_US 2000u

/* MAX7219 register addresses */
#define MAX7219_REG_DIGIT0      0x01u
#define MAX7219_REG_DECODE_MODE 0x09u
#define MAX7219_REG_INTENSITY   0x0Au
#define MAX7219_REG_SCAN_LIMIT  0x0Bu
#define MAX7219_REG_SHUTDOWN    0x0Cu
#define MAX7219_REG_DISP_TEST   0x0Fu

/* MAX7219 no-decode segment bit mapping:
 *   MAX7219 bit 0 = DP
 *   MAX7219 bit 1 = A … bit 7 = G
 * Internal encoding: bit 0 = A … bit 6 = G, bit 7 = DP
 * Conversion: max_byte = ((internal & 0x7F) << 1) | ((internal >> 7) & 1)
 */
#define INTERNAL_TO_MAX7219(b) ((uint8_t)(((b) & 0x7Fu) << 1u) | (uint8_t)(((b) >> 7u) & 1u))

/* MAX7219 SPI clock speed (10 MHz max per datasheet) */
#define MAX7219_SPI_CLK_HZ 10000000

/* ---------------------------------------------------------------------------
 * Handle definition
 * --------------------------------------------------------------------------- */

struct blusys_seven_seg {
    blusys_seven_seg_driver_t driver;
    blusys_lock_t             lock;
    uint8_t                   digit_count;

    /* Segment bitmask for each digit (index 0 = leftmost).
     * Written under lock; read by mux timer without lock (byte-atomic). */
    uint8_t buf[BLUSYS_SEVEN_SEG_MAX_DIGITS];

    /* GPIO / 74HC595 shared fields */
    int                common_pins[BLUSYS_SEVEN_SEG_MAX_DIGITS];
    bool               common_anode;
    esp_timer_handle_t mux_timer;
    volatile uint8_t   active_digit;

    /* GPIO-specific: a, b, c, d, e, f, g, dp (-1 = not wired) */
    int seg_pins[8];

    /* 74HC595-specific */
    int data_pin;
    int clk_pin;
    int latch_pin;

    /* MAX7219-specific */
    spi_device_handle_t spi_dev;
};

/* ---------------------------------------------------------------------------
 * Helpers — MAX7219 SPI
 * --------------------------------------------------------------------------- */

static blusys_err_t max7219_write(blusys_seven_seg_t *display, uint8_t reg, uint8_t data)
{
    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = NULL,
        .flags     = SPI_TRANS_USE_TXDATA,
    };
    t.tx_data[0] = reg;
    t.tx_data[1] = data;
    esp_err_t err = spi_device_transmit(display->spi_dev, &t);
    return blusys_translate_esp_err(err);
}

static blusys_err_t max7219_flush(blusys_seven_seg_t *display)
{
    blusys_err_t err;
    for (uint8_t i = 0; i < display->digit_count; i++) {
        /* MAX7219 digit registers are 1-indexed; digit0 = leftmost */
        err = max7219_write(display,
                            (uint8_t)(MAX7219_REG_DIGIT0 + i),
                            INTERNAL_TO_MAX7219(display->buf[i]));
        if (err != BLUSYS_OK) {
            return err;
        }
    }
    return BLUSYS_OK;
}

/* ---------------------------------------------------------------------------
 * Helpers — 74HC595 bit-bang
 * --------------------------------------------------------------------------- */

static void hc595_shift_out(const blusys_seven_seg_t *display, uint8_t byte)
{
    for (int i = 7; i >= 0; i--) {
        gpio_set_level((gpio_num_t)display->data_pin, (byte >> i) & 1u);
        gpio_set_level((gpio_num_t)display->clk_pin, 1);
        gpio_set_level((gpio_num_t)display->clk_pin, 0);
    }
    gpio_set_level((gpio_num_t)display->latch_pin, 1);
    gpio_set_level((gpio_num_t)display->latch_pin, 0);
}

/* ---------------------------------------------------------------------------
 * Multiplexing timer callback (GPIO and 74HC595 only)
 *
 * Runs in the esp_timer task — not a true ISR, so GPIO calls are safe.
 * Reads buf[] without holding the lock; byte-wide reads are atomic on
 * Xtensa and RISC-V so this is safe with the lock-protected writes in the
 * public API.
 * --------------------------------------------------------------------------- */

static void mux_timer_cb(void *arg)
{
    blusys_seven_seg_t *display = (blusys_seven_seg_t *)arg;
    uint8_t cur  = display->active_digit;
    uint8_t next = (uint8_t)((cur + 1u) % display->digit_count);

    /* Disable the currently active digit */
    gpio_set_level((gpio_num_t)display->common_pins[cur],
                   display->common_anode ? 0 : 1);

    /* Drive segments for the next digit */
    uint8_t segs = display->buf[next];

    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_GPIO) {
        for (int i = 0; i < 8; i++) {
            if (display->seg_pins[i] == -1) {
                continue;
            }
            int level = (segs >> i) & 1;
            if (display->common_anode) {
                level = level ? 0 : 1; /* active-low for common anode */
            }
            gpio_set_level((gpio_num_t)display->seg_pins[i], level);
        }
    } else {
        /* 74HC595: active-low segments for common anode, active-high for common cathode */
        uint8_t out = display->common_anode ? (uint8_t)~segs : segs;
        hc595_shift_out(display, out);
    }

    /* Enable the next digit */
    gpio_set_level((gpio_num_t)display->common_pins[next],
                   display->common_anode ? 1 : 0);

    display->active_digit = next;
}

/* ---------------------------------------------------------------------------
 * GPIO pin setup helpers
 * --------------------------------------------------------------------------- */

static blusys_err_t configure_output(int pin)
{
    if (pin == -1) {
        return BLUSYS_OK;
    }
    return blusys_gpio_set_output(pin);
}

static void reset_output(int pin)
{
    if (pin == -1) {
        return;
    }
    blusys_gpio_reset(pin);
}

/* ---------------------------------------------------------------------------
 * Driver open helpers
 * --------------------------------------------------------------------------- */

static blusys_err_t open_gpio(blusys_seven_seg_t *display,
                               const blusys_seven_seg_gpio_config_t *cfg)
{
    blusys_err_t err;
    int seg_pins_src[8] = {
        cfg->pin_a, cfg->pin_b, cfg->pin_c, cfg->pin_d,
        cfg->pin_e, cfg->pin_f, cfg->pin_g, cfg->pin_dp,
    };

    for (int i = 0; i < 8; i++) {
        display->seg_pins[i] = seg_pins_src[i];
        err = configure_output(seg_pins_src[i]);
        if (err != BLUSYS_OK) {
            return err;
        }
        if (seg_pins_src[i] != -1) {
            /* All segments off initially */
            gpio_set_level((gpio_num_t)seg_pins_src[i], cfg->common_anode ? 1 : 0);
        }
    }

    for (uint8_t i = 0; i < display->digit_count; i++) {
        if (cfg->common_pins[i] == -1) {
            return BLUSYS_ERR_INVALID_ARG; /* every active digit needs a common pin */
        }
        display->common_pins[i] = cfg->common_pins[i];
        err = configure_output(cfg->common_pins[i]);
        if (err != BLUSYS_OK) {
            return err;
        }
        gpio_set_level((gpio_num_t)cfg->common_pins[i], cfg->common_anode ? 0 : 1);
    }

    display->common_anode = cfg->common_anode;
    return BLUSYS_OK;
}

static blusys_err_t open_hc595(blusys_seven_seg_t *display,
                                const blusys_seven_seg_595_config_t *cfg)
{
    blusys_err_t err;

    display->data_pin  = cfg->data_pin;
    display->clk_pin   = cfg->clk_pin;
    display->latch_pin = cfg->latch_pin;

    int shift_pins[3] = { cfg->data_pin, cfg->clk_pin, cfg->latch_pin };
    for (int i = 0; i < 3; i++) {
        err = configure_output(shift_pins[i]);
        if (err != BLUSYS_OK) {
            return err;
        }
        gpio_set_level((gpio_num_t)shift_pins[i], 0);
    }

    /* Start with all segments off (shift out blank) */
    uint8_t blank = cfg->common_anode ? 0xFFu : 0x00u;
    hc595_shift_out(display, blank);

    for (uint8_t i = 0; i < display->digit_count; i++) {
        if (cfg->common_pins[i] == -1) {
            return BLUSYS_ERR_INVALID_ARG; /* every active digit needs a common pin */
        }
        display->common_pins[i] = cfg->common_pins[i];
        err = configure_output(cfg->common_pins[i]);
        if (err != BLUSYS_OK) {
            return err;
        }
        gpio_set_level((gpio_num_t)cfg->common_pins[i], cfg->common_anode ? 0 : 1);
    }

    display->common_anode = cfg->common_anode;
    return BLUSYS_OK;
}

static blusys_err_t open_max7219(blusys_seven_seg_t *display,
                                  const blusys_seven_seg_max7219_config_t *cfg,
                                  uint8_t digit_count)
{
    esp_err_t esp_err;
    blusys_err_t err;

    if (!GPIO_IS_VALID_OUTPUT_GPIO(cfg->mosi_pin) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(cfg->clk_pin) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(cfg->cs_pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = cfg->mosi_pin,
        .miso_io_num     = -1,
        .sclk_io_num     = cfg->clk_pin,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 2,
    };
    esp_err = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = MAX7219_SPI_CLK_HZ,
        .mode           = 0,
        .spics_io_num   = cfg->cs_pin,
        .queue_size     = 1,
    };
    esp_err = spi_bus_add_device(SPI2_HOST, &dev_cfg, &display->spi_dev);
    if (esp_err != ESP_OK) {
        spi_bus_free(SPI2_HOST);
        return blusys_translate_esp_err(esp_err);
    }

    /* Initialise MAX7219 */
    err = max7219_write(display, MAX7219_REG_DISP_TEST,   0x00); /* normal mode */
    if (err != BLUSYS_OK) { goto fail_dev; }
    err = max7219_write(display, MAX7219_REG_DECODE_MODE, 0x00); /* no BCD decode */
    if (err != BLUSYS_OK) { goto fail_dev; }
    err = max7219_write(display, MAX7219_REG_SCAN_LIMIT,  (uint8_t)(digit_count - 1u));
    if (err != BLUSYS_OK) { goto fail_dev; }
    err = max7219_write(display, MAX7219_REG_INTENSITY,   0x08); /* mid brightness */
    if (err != BLUSYS_OK) { goto fail_dev; }

    /* Blank all digits before enabling display */
    for (uint8_t i = 0; i < digit_count; i++) {
        err = max7219_write(display, (uint8_t)(MAX7219_REG_DIGIT0 + i), 0x00);
        if (err != BLUSYS_OK) { goto fail_dev; }
    }

    err = max7219_write(display, MAX7219_REG_SHUTDOWN, 0x01); /* normal operation */
    if (err != BLUSYS_OK) { goto fail_dev; }

    return BLUSYS_OK;

fail_dev:
    spi_bus_remove_device(display->spi_dev);
    spi_bus_free(SPI2_HOST);
    display->spi_dev = NULL;
    return err;
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

blusys_err_t blusys_seven_seg_open(const blusys_seven_seg_config_t *config,
                                   blusys_seven_seg_t **out_display)
{
    blusys_err_t err;

    if ((config == NULL) || (out_display == NULL) ||
        (config->digit_count == 0u) ||
        (config->digit_count > BLUSYS_SEVEN_SEG_MAX_DIGITS)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_seven_seg_t *display = calloc(1, sizeof(*display));
    if (display == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    display->driver      = config->driver;
    display->digit_count = config->digit_count;
    display->spi_dev     = NULL;
    display->mux_timer   = NULL;

    /* Initialise seg_pins to -1 so the mux callback skips un-wired pins */
    memset(display->seg_pins,    -1, sizeof(display->seg_pins));
    memset(display->common_pins, -1, sizeof(display->common_pins));

    err = blusys_lock_init(&display->lock);
    if (err != BLUSYS_OK) {
        free(display);
        return err;
    }

    switch (config->driver) {
    case BLUSYS_SEVEN_SEG_DRIVER_GPIO:
        err = open_gpio(display, &config->gpio);
        break;
    case BLUSYS_SEVEN_SEG_DRIVER_74HC595:
        err = open_hc595(display, &config->hc595);
        break;
    case BLUSYS_SEVEN_SEG_DRIVER_MAX7219:
        err = open_max7219(display, &config->max7219, config->digit_count);
        break;
    default:
        err = BLUSYS_ERR_INVALID_ARG;
        break;
    }

    if (err != BLUSYS_OK) {
        goto fail_driver;
    }

    /* GPIO and 74HC595 need a software multiplexing timer */
    if (config->driver != BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        esp_timer_create_args_t timer_args = {
            .callback        = mux_timer_cb,
            .arg             = display,
            .dispatch_method = ESP_TIMER_TASK,
            .name            = "seven_seg_mux",
        };
        esp_err_t esp_err = esp_timer_create(&timer_args, &display->mux_timer);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            goto fail_driver;
        }
        esp_err = esp_timer_start_periodic(display->mux_timer, MUX_PERIOD_US);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            esp_timer_delete(display->mux_timer);
            display->mux_timer = NULL;
            goto fail_driver;
        }
    }

    *out_display = display;
    return BLUSYS_OK;

fail_driver:
    blusys_lock_deinit(&display->lock);
    free(display);
    return err;
}

blusys_err_t blusys_seven_seg_close(blusys_seven_seg_t *display)
{
    blusys_err_t err;

    if (display == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Stop the mux timer before taking the lock. esp_timer_stop waits for any
     * in-progress callback to complete; the callback never takes the lock so
     * there is no deadlock risk. Stopping first prevents the callback from
     * accessing the handle after we free it. */
    if (display->mux_timer != NULL) {
        esp_timer_stop(display->mux_timer);
        esp_timer_delete(display->mux_timer);
        display->mux_timer = NULL;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        max7219_write(display, MAX7219_REG_SHUTDOWN, 0x00); /* shutdown */
        spi_bus_remove_device(display->spi_dev);
        spi_bus_free(SPI2_HOST);
    } else if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_GPIO) {
        for (int i = 0; i < 8; i++) {
            reset_output(display->seg_pins[i]);
        }
        for (uint8_t i = 0; i < display->digit_count; i++) {
            reset_output(display->common_pins[i]);
        }
    } else {
        /* 74HC595 */
        reset_output(display->data_pin);
        reset_output(display->clk_pin);
        reset_output(display->latch_pin);
        for (uint8_t i = 0; i < display->digit_count; i++) {
            reset_output(display->common_pins[i]);
        }
    }

    blusys_lock_give(&display->lock);
    blusys_lock_deinit(&display->lock);
    free(display);
    return BLUSYS_OK;
}

blusys_err_t blusys_seven_seg_write_int(blusys_seven_seg_t *display,
                                        int value, bool leading_zeros)
{
    blusys_err_t err;

    if (display == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    bool negative = (value < 0);
    if (value == INT_MIN) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    unsigned int uval = negative ? (unsigned int)(-value) : (unsigned int)value;

    /* Verify the value fits.
     * For negative numbers we need one digit for the minus sign. */
    unsigned int max_digits = display->digit_count;
    if (negative) {
        max_digits--;
    }
    unsigned int limit = 1;
    for (unsigned int i = 0; i < max_digits; i++) {
        limit *= 10u;
    }
    if (uval >= limit) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    uint8_t newbuf[BLUSYS_SEVEN_SEG_MAX_DIGITS];
    memset(newbuf, leading_zeros ? s_segments[0] : SEG_BLANK, sizeof(newbuf));

    /* Fill digits right-to-left */
    int pos = (int)display->digit_count - 1;
    if (uval == 0 && !negative) {
        newbuf[pos] = s_segments[0];
        pos--;
    } else {
        unsigned int v = uval;
        while (v > 0u && pos >= 0) {
            newbuf[pos] = s_segments[v % 10u];
            v /= 10u;
            pos--;
        }
    }

    if (negative && pos >= 0) {
        newbuf[pos] = SEG_MINUS;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    memcpy(display->buf, newbuf, display->digit_count);

    err = BLUSYS_OK;
    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        err = max7219_flush(display);
    }

    blusys_lock_give(&display->lock);
    return err;
}

blusys_err_t blusys_seven_seg_write_hex(blusys_seven_seg_t *display, uint32_t value)
{
    blusys_err_t err;

    if (display == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* Verify value fits in digit_count hex digits.
     * Use uint64_t: 16^8 = 4,294,967,296 overflows uint32_t. */
    uint64_t max_val = 1u;
    for (uint8_t i = 0; i < display->digit_count; i++) {
        max_val *= 16u;
    }
    if ((uint64_t)value >= max_val) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    uint8_t newbuf[BLUSYS_SEVEN_SEG_MAX_DIGITS];
    uint32_t v = value;
    for (int i = (int)display->digit_count - 1; i >= 0; i--) {
        newbuf[i] = s_segments[v & 0xFu];
        v >>= 4u;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    memcpy(display->buf, newbuf, display->digit_count);

    err = BLUSYS_OK;
    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        err = max7219_flush(display);
    }

    blusys_lock_give(&display->lock);
    return err;
}

blusys_err_t blusys_seven_seg_write_raw(blusys_seven_seg_t *display,
                                        const uint8_t *segments, uint8_t count)
{
    blusys_err_t err;

    if ((display == NULL) || (segments == NULL) ||
        (count == 0u) || (count > display->digit_count)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    memcpy(display->buf, segments, count);

    err = BLUSYS_OK;
    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        err = max7219_flush(display);
    }

    blusys_lock_give(&display->lock);
    return err;
}

blusys_err_t blusys_seven_seg_set_dp(blusys_seven_seg_t *display, uint8_t digit_mask)
{
    blusys_err_t err;

    if (display == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    for (uint8_t i = 0; i < display->digit_count; i++) {
        if ((digit_mask >> i) & 1u) {
            display->buf[i] |= 0x80u;
        } else {
            display->buf[i] &= 0x7Fu;
        }
    }

    err = BLUSYS_OK;
    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        err = max7219_flush(display);
    }

    blusys_lock_give(&display->lock);
    return err;
}

blusys_err_t blusys_seven_seg_set_brightness(blusys_seven_seg_t *display, uint8_t level)
{
    blusys_err_t err;

    if (display == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (display->driver != BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }
    if (level > 15u) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = max7219_write(display, MAX7219_REG_INTENSITY, level);

    blusys_lock_give(&display->lock);
    return err;
}

blusys_err_t blusys_seven_seg_clear(blusys_seven_seg_t *display)
{
    blusys_err_t err;

    if (display == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&display->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    memset(display->buf, SEG_BLANK, display->digit_count);

    err = BLUSYS_OK;
    if (display->driver == BLUSYS_SEVEN_SEG_DRIVER_MAX7219) {
        err = max7219_flush(display);
    }

    blusys_lock_give(&display->lock);
    return err;
}
