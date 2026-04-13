# LCD

SPI and I2C display driver supporting ST7789, ST7735, SSD1306, NT35510, ILI9341, and ILI9488 panels via ESP-IDF's `esp_lcd`. A QEMU virtual-framebuffer driver (`BLUSYS_LCD_DRIVER_QEMU_RGB`) is also available for QEMU-only builds.

## Quick Example

```c
#include "blusys/drivers/display/lcd.h"

static uint16_t row_buf[240];

void app_main(void)
{
    blusys_lcd_t *lcd;

    blusys_lcd_config_t config = {
        .driver         = BLUSYS_LCD_DRIVER_ST7789,
        .width          = 240,
        .height         = 320,
        .bits_per_pixel = 16,
        .bgr_order      = false,
        .spi = {
            .bus      = 0,
            .sclk_pin = 12,
            .mosi_pin = 11,
            .cs_pin   = 10,
            .dc_pin   = 13,
            .rst_pin  = -1,
            .bl_pin   = -1,
            .pclk_hz  = 20000000,
        },
    };

    blusys_lcd_open(&config, &lcd);

    /* fill red (RGB565) */
    for (int i = 0; i < 240; i++) {
        row_buf[i] = 0xF800;
    }
    for (int row = 0; row < 320; row++) {
        blusys_lcd_draw_bitmap(lcd, 0, row, 240, row + 1, row_buf);
    }

    blusys_lcd_close(lcd);
}
```

## Common Mistakes

- **Wrong DC pin** — the Data/Command pin must be a valid output GPIO and correctly wired; swapping it with another signal produces a blank or garbled screen
- **BGR vs RGB** — if colors appear swapped (red shows as blue), set `bgr_order = true`
- **SPI bus conflict** — `blusys_lcd_open()` takes exclusive ownership of the SPI bus; do not call `blusys_spi_open()` on the same bus index
- **I2C bus conflict** — `blusys_lcd_open()` takes exclusive ownership of the I2C bus; do not call `blusys_i2c_open()` on the same port
- **Wrong I2C address** — SSD1306 modules use either `0x3C` or `0x3D`; if the screen stays blank, try the other address
- **Missing reset wiring** — some panels require the RST line to be connected; if the display stays blank, wire RST and set `rst_pin` to the correct GPIO

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_lcd_t`

```c
typedef struct blusys_lcd blusys_lcd_t;
```

Opaque handle returned by `blusys_lcd_open()`.

### `blusys_lcd_driver_t`

```c
typedef enum {
    BLUSYS_LCD_DRIVER_ST7789   = 0,
    BLUSYS_LCD_DRIVER_SSD1306  = 1,
    BLUSYS_LCD_DRIVER_NT35510  = 2,
    BLUSYS_LCD_DRIVER_ST7735   = 3,
    BLUSYS_LCD_DRIVER_ILI9341  = 4,
    BLUSYS_LCD_DRIVER_ILI9488  = 5,
    /* ESP-IDF QEMU virtual framebuffer (esp_lcd_qemu_rgb); QEMU only */
    BLUSYS_LCD_DRIVER_QEMU_RGB = 6,
} blusys_lcd_driver_t;
```

### `blusys_lcd_spi_config_t`

```c
typedef struct {
    int      bus;       /* SPI host: 0 = SPI2_HOST, 1 = SPI3_HOST */
    int      sclk_pin;
    int      mosi_pin;
    int      cs_pin;
    int      dc_pin;    /* Data/Command select */
    int      rst_pin;   /* -1 if not connected */
    int      bl_pin;    /* Backlight GPIO, -1 if not used */
    uint32_t pclk_hz;
    int      x_offset;  /* GRAM column offset (default 0) */
    int      y_offset;  /* GRAM row offset (default 0) */

    /* Optional LEDC PWM backlight dimming. When bl_ledc_enabled is false
     * (the default), the driver drives bl_pin as a plain GPIO and
     * blusys_lcd_set_brightness() maps any non-zero value to full on.
     * When true, the driver programs LEDC at open time and
     * blusys_lcd_set_brightness() maps 0–100 linearly to the PWM duty. */
    bool     bl_ledc_enabled;
    int      bl_ledc_channel;  /* LEDC channel 0–7 */
    int      bl_ledc_timer;    /* LEDC timer   0–3 */
    uint32_t bl_ledc_freq_hz;  /* PWM frequency (0 → 5000 Hz) */
} blusys_lcd_spi_config_t;
```

### `blusys_lcd_i2c_config_t`

```c
typedef struct {
    int      port;      /* I2C port number */
    int      sda_pin;
    int      scl_pin;
    uint8_t  dev_addr;  /* 0x3C or 0x3D */
    int      rst_pin;   /* -1 if not connected */
    uint32_t freq_hz;
} blusys_lcd_i2c_config_t;
```

### `blusys_lcd_config_t`

```c
typedef struct {
    blusys_lcd_driver_t driver;
    uint32_t            width;
    uint32_t            height;
    uint8_t             bits_per_pixel; /* 16 = RGB565, 1 = SSD1306 mono */
    bool                bgr_order;
    bool                swap_xy;      /* Apply axis swap during init */
    bool                mirror_x;     /* Apply X mirror during init */
    bool                mirror_y;     /* Apply Y mirror during init */
    bool                invert_color; /* Apply panel color inversion during init */
    union {
        blusys_lcd_spi_config_t spi;
        blusys_lcd_i2c_config_t i2c;
    };
} blusys_lcd_config_t;
```

Use the `spi` union member for ST7789, ST7735, NT35510, ILI9341, and ILI9488 drivers. Use the `i2c` union member for the SSD1306 driver. The `QEMU_RGB` driver is selected only in QEMU builds and ignores transport pins.

Set `swap_xy`, `mirror_x`, `mirror_y`, and `invert_color` when a panel needs a specific orientation or hardware color inversion immediately after init. For common 128x160 ST7735 modules, portrait mode is often `swap_xy = true`, `mirror_x = true`, `mirror_y = false`, `invert_color = false`.

## Functions

### `blusys_lcd_open`

```c
blusys_err_t blusys_lcd_open(const blusys_lcd_config_t *config,
                             blusys_lcd_t **out_lcd);
```

Creates the display transport (SPI or I2C bus), resets and initializes the panel driver, applies any configured gap/orientation/inversion settings, and turns it on. If `bl_pin` is set, the backlight GPIO is driven high.

**Parameters:**

- `config` — display configuration (required)
- `out_lcd` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if config or pins are invalid, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_INVALID_STATE` if the SPI/I2C bus is already in use.

---

### `blusys_lcd_close`

```c
blusys_err_t blusys_lcd_close(blusys_lcd_t *lcd);
```

Turns the display off, releases the backlight GPIO, tears down the panel driver and bus, and frees the handle. After this call the handle is invalid.

---

### `blusys_lcd_draw_bitmap`

```c
blusys_err_t blusys_lcd_draw_bitmap(blusys_lcd_t *lcd,
                                    int x_start, int y_start,
                                    int x_end, int y_end,
                                    const void *color_data);
```

Draws pixel data into the rectangle `[x_start, x_end) x [y_start, y_end)`. The `color_data` buffer must contain `(x_end - x_start) * (y_end - y_start)` pixels in the format matching `bits_per_pixel`. Blocking — the call returns after the transfer completes.

**Parameters:**

- `lcd` — handle from `blusys_lcd_open()`
- `x_start`, `y_start` — top-left corner (inclusive)
- `x_end`, `y_end` — bottom-right corner (exclusive)
- `color_data` — pixel buffer

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if coordinates are invalid.

---

### `blusys_lcd_on`

```c
blusys_err_t blusys_lcd_on(blusys_lcd_t *lcd);
```

Turns the display output on. Does not affect the backlight GPIO.

---

### `blusys_lcd_off`

```c
blusys_err_t blusys_lcd_off(blusys_lcd_t *lcd);
```

Turns the display output off. The framebuffer contents are preserved.

---

### `blusys_lcd_mirror`

```c
blusys_err_t blusys_lcd_mirror(blusys_lcd_t *lcd, bool mirror_x, bool mirror_y);
```

Mirrors the display output along the X and/or Y axis.

---

### `blusys_lcd_swap_xy`

```c
blusys_err_t blusys_lcd_swap_xy(blusys_lcd_t *lcd, bool swap);
```

Swaps the X and Y axes. Combined with `blusys_lcd_mirror()` this enables 90/180/270 degree rotation.

---

### `blusys_lcd_invert_colors`

```c
blusys_err_t blusys_lcd_invert_colors(blusys_lcd_t *lcd, bool invert);
```

Inverts all displayed colors.

---

### `blusys_lcd_set_brightness`

```c
blusys_err_t blusys_lcd_set_brightness(blusys_lcd_t *lcd, int percent);
```

Controls the backlight. When `bl_ledc_enabled` is false, the driver uses plain GPIO on/off: any `percent > 0` turns the backlight on, `0` turns it off. When `bl_ledc_enabled` is true, `percent` is mapped linearly (0–100) to the LEDC PWM duty. No-op if `bl_pin` was set to `-1`.

**Parameters:**

- `percent` — 0 to 100

---

### `blusys_lcd_get_dimensions`

```c
blusys_err_t blusys_lcd_get_dimensions(blusys_lcd_t *lcd,
                                       uint32_t *width, uint32_t *height);
```

Returns the panel dimensions configured at open time. Either `width` or `height` may be NULL if only one dimension is needed.

**Parameters:**

- `lcd` — handle returned by `blusys_lcd_open()`
- `width` — receives the panel width in pixels; may be NULL
- `height` — receives the panel height in pixels; may be NULL

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `lcd` is NULL or both `width` and `height` are NULL.

## Lifecycle

1. Fill a `blusys_lcd_config_t` with driver type, dimensions, and transport pins
2. `blusys_lcd_open()` — allocates, initializes hardware, resets and enables the display
3. `blusys_lcd_draw_bitmap()` — render content
4. `blusys_lcd_close()` — turns off display, releases all resources

## Thread Safety

All functions are thread-safe via an internal mutex. Note that `blusys_lcd_draw_bitmap()` holds the lock for the full duration of the pixel transfer, which can be tens of milliseconds for large regions. Other callers on the same handle will block during this time.

## Limitations

- The LCD module takes exclusive ownership of the SPI or I2C bus. Do not use the same bus index with `blusys_spi_open()` or `blusys_i2c_open()`.
- Backlight control defaults to GPIO on/off. Opt in to PWM dimming by setting `bl_ledc_enabled = true` and picking a free LEDC timer/channel.
- Coordinates passed to `blusys_lcd_draw_bitmap()` must be within the panel dimensions; no automatic clipping is performed.
- On ST7735 panels over SPI, the driver issues **one display row per RAMWR** command (column/row address window is re-asserted per row) to avoid diagonal shear observed with multi-row pixel bursts. See `docs/internals/architecture.md#display-notes-st7735-on-spi`.

## Example App

See `examples/reference/lcd_basic/` (ST7789), `examples/reference/lcd_st7735_basic/` (ST7735R 128x160), and `examples/validation/lcd_ssd1306_basic/` (SSD1306 128x32 I2C OLED).
