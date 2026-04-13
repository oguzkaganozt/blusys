# I2S

Stereo audio streaming over I2S: TX for playback, RX for capture. Fixed to standard Philips mode, 16-bit stereo.

## Quick Example

```c
#include <stdint.h>

#include "blusys/i2s.h"

void app_main(void)
{
    blusys_i2s_tx_t *i2s;
    int16_t audio[] = {12000, 12000, -12000, -12000};

    if (blusys_i2s_tx_open(&(blusys_i2s_tx_config_t) {
                               .port = 0,
                               .bclk_pin = 4,
                               .ws_pin = 5,
                               .dout_pin = 6,
                               .mclk_pin = -1,
                               .sample_rate_hz = 16000u,
                           },
                           &i2s) != BLUSYS_OK) {
        return;
    }

    blusys_i2s_tx_start(i2s);
    blusys_i2s_tx_write(i2s, audio, sizeof(audio), 1000);
    blusys_i2s_tx_stop(i2s);
    blusys_i2s_tx_close(i2s);
}
```

## Common Mistakes

- expecting useful analog output without an external I2S DAC or codec
- wiring the external device to a different sample format than stereo 16-bit Philips mode
- calling `blusys_i2s_tx_write()` before `blusys_i2s_tx_start()`
- assuming full-duplex support exists — TX and RX are separate channels and cannot share a single port

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_i2s_tx_t`

```c
typedef struct blusys_i2s_tx blusys_i2s_tx_t;
```

Opaque TX handle returned by `blusys_i2s_tx_open()`.

### `blusys_i2s_tx_config_t`

```c
typedef struct {
    int port;              /* I2S controller index */
    int bclk_pin;          /* bit clock GPIO */
    int ws_pin;            /* word select (LRCLK) GPIO */
    int dout_pin;          /* data out GPIO */
    int mclk_pin;          /* master clock GPIO; set to -1 if unused */
    uint32_t sample_rate_hz; /* audio sample rate (e.g. 44100) */
} blusys_i2s_tx_config_t;
```

### `blusys_i2s_rx_t`

```c
typedef struct blusys_i2s_rx blusys_i2s_rx_t;
```

Opaque RX handle returned by `blusys_i2s_rx_open()`.

### `blusys_i2s_rx_config_t`

```c
typedef struct {
    int port;              /* I2S controller index */
    int bclk_pin;          /* bit clock GPIO */
    int ws_pin;            /* word select (LRCLK) GPIO */
    int din_pin;           /* data in GPIO */
    int mclk_pin;          /* master clock GPIO; set to -1 if unused */
    uint32_t sample_rate_hz; /* audio sample rate */
} blusys_i2s_rx_config_t;
```

## Functions — TX

### `blusys_i2s_tx_open`

```c
blusys_err_t blusys_i2s_tx_open(const blusys_i2s_tx_config_t *config,
                                blusys_i2s_tx_t **out_i2s);
```

Opens an I2S TX channel. Output is not active until `blusys_i2s_tx_start()`.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid config (NULL, invalid pins, zero sample rate).

---

### `blusys_i2s_tx_close`

```c
blusys_err_t blusys_i2s_tx_close(blusys_i2s_tx_t *i2s);
```

Stops and releases the TX channel.

---

### `blusys_i2s_tx_start`

```c
blusys_err_t blusys_i2s_tx_start(blusys_i2s_tx_t *i2s);
```

Enables the I2S clock and starts DMA transfer.

---

### `blusys_i2s_tx_stop`

```c
blusys_err_t blusys_i2s_tx_stop(blusys_i2s_tx_t *i2s);
```

Stops DMA transfer and disables the clock.

---

### `blusys_i2s_tx_write`

```c
blusys_err_t blusys_i2s_tx_write(blusys_i2s_tx_t *i2s,
                                 const void *data,
                                 size_t size,
                                 int timeout_ms);
```

Writes interleaved 16-bit stereo samples. Provide L and R samples alternating: `[L0, R0, L1, R1, ...]`.

**Parameters:**
- `i2s` — TX handle
- `data` — sample buffer
- `size` — buffer size in bytes
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if called before `start()`, `BLUSYS_ERR_TIMEOUT`.

## Functions — RX

### `blusys_i2s_rx_open`

```c
blusys_err_t blusys_i2s_rx_open(const blusys_i2s_rx_config_t *config,
                                blusys_i2s_rx_t **out_i2s);
```

Opens an I2S RX channel. Capture does not begin until `blusys_i2s_rx_start()`.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid config.

---

### `blusys_i2s_rx_close`

```c
blusys_err_t blusys_i2s_rx_close(blusys_i2s_rx_t *i2s);
```

Stops and releases the RX channel.

---

### `blusys_i2s_rx_start`

```c
blusys_err_t blusys_i2s_rx_start(blusys_i2s_rx_t *i2s);
```

Enables the I2S clock and begins capture.

---

### `blusys_i2s_rx_stop`

```c
blusys_err_t blusys_i2s_rx_stop(blusys_i2s_rx_t *i2s);
```

Stops capture and disables the clock.

---

### `blusys_i2s_rx_read`

```c
blusys_err_t blusys_i2s_rx_read(blusys_i2s_rx_t *i2s,
                                void *buf,
                                size_t size,
                                size_t *bytes_read,
                                int timeout_ms);
```

Reads captured audio samples into `buf`. Returns interleaved 16-bit stereo samples.

**Parameters:**
- `i2s` — RX handle
- `buf` — destination buffer
- `size` — buffer size in bytes
- `bytes_read` — actual bytes received
- `timeout_ms` — milliseconds to wait

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`.

## Lifecycle

**TX:** `open()` → `start()` → `write()` → `stop()` → `close()`

**RX:** `open()` → `start()` → `read()` → `stop()` → `close()`

## Thread Safety

- concurrent calls on the same handle are serialized internally
- TX and RX handles are independent
- do not call close concurrently with other calls on the same handle

## Limitations

- format is fixed: Philips standard mode, master mode, stereo, 16-bit samples
- an external I2S DAC or ADC/microphone is required
- mono, TDM, PDM, slave mode, and duplex are not supported
- slot width, data width, and advanced clock tuning are not exposed

## Example App

See `examples/validation/i2s_basic/` for TX.
See `examples/validation/i2s_rx_basic/` for RX.
