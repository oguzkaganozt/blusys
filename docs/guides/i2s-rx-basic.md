# Receive Audio Samples Over I2S

## Problem Statement

You want to capture audio or digital samples from an I2S microphone or other I2S source (e.g. INMP441, SPH0645) into a buffer.

## Prerequisites

- a supported board
- an I2S microphone or I2S source wired to BCLK, WS, and DIN pins
- the I2S source configured as clock slave (the ESP32 drives BCLK and WS)

## Minimal Example

```c
blusys_i2s_rx_t *i2s;
int16_t buf[256 * 2];  /* 256 stereo frames, L+R interleaved */
size_t bytes_read;

blusys_i2s_rx_config_t config = {
    .port           = 0,
    .bclk_pin       = 14,
    .ws_pin         = 15,
    .din_pin        = 13,
    .mclk_pin       = -1,
    .sample_rate_hz = 16000,
};

blusys_i2s_rx_open(&config, &i2s);
blusys_i2s_rx_start(i2s);

blusys_i2s_rx_read(i2s, buf, sizeof(buf), &bytes_read, 1000);

blusys_i2s_rx_stop(i2s);
blusys_i2s_rx_close(i2s);
```

## APIs Used

- `blusys_i2s_rx_open()` configures the I2S port, clock pins, data input pin, optional MCLK, and sample rate
- `blusys_i2s_rx_start()` enables the channel and begins clocking the I2S bus
- `blusys_i2s_rx_read()` blocks until the buffer is filled or the timeout expires
- `blusys_i2s_rx_stop()` disables the channel without releasing resources
- `blusys_i2s_rx_close()` releases the channel handle

## Buffer Format

The channel is configured as 16-bit stereo Philips (I2S standard) mode. Each read fills the buffer with interleaved left and right channel samples:

```
buf[0] = L[0], buf[1] = R[0], buf[2] = L[1], buf[3] = R[1], ...
```

Buffer size must be a multiple of 4 bytes (one stereo frame = 2 × int16_t).

## MCLK

Pass `mclk_pin = -1` if the microphone does not need a master clock. Some microphones (e.g. SPH0645) require MCLK; set it to a free GPIO in that case.

## Expected Runtime Behavior

- `start()` begins clocking; the microphone starts streaming immediately
- `read()` returns when the buffer is full
- if no data arrives within `timeout_ms`, `read()` returns `BLUSYS_ERR_TIMEOUT`
- `stop()` / `start()` can be called repeatedly without closing the handle

## Common Mistakes

- omitting `blusys_i2s_rx_start()` before `read()` — `read()` will block indefinitely
- passing a buffer size not aligned to 4 bytes — the ESP-IDF I2S driver may truncate or error
- using this on a microphone that requires I2S master mode while forgetting that the ESP32 always drives the clock in this configuration

## Example App

See `examples/i2s_rx_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [I2S API Reference](../modules/i2s.md).
