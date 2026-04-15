# Sigma-Delta Modulation

High-frequency sigma-delta bitstream output on a GPIO. Useful for LED dimming, simple DAC approximation with an RC filter, or any load that accepts a PWM-style bitstream.

## Quick Example

```c
blusys_sdm_t *sdm;

blusys_sdm_open(8, 1000000, &sdm);   /* 1 MHz sample rate */

/* sweep from low to high and back */
for (int8_t d = -90; d <= 90; d += 10) {
    blusys_sdm_set_density(sdm, d);
    vTaskDelay(pdMS_TO_TICKS(200));
}

blusys_sdm_close(sdm);
```

## Common Mistakes

- omitting the RC filter when a smooth voltage is needed; without it, the output is a high-speed digital signal
- confusing SDM with LEDC PWM: both produce variable duty-cycle signals, but SDM uses a fixed frequency and is better suited for audio or high-resolution DC approximation

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_sdm_t`

```c
typedef struct blusys_sdm blusys_sdm_t;
```

Opaque handle returned by `blusys_sdm_open()`.

## Functions

### `blusys_sdm_open`

```c
blusys_err_t blusys_sdm_open(int pin, uint32_t sample_rate_hz, blusys_sdm_t **out_sdm);
```

Opens an SDM channel on the given pin and enables the output immediately.

**Parameters:**
- `pin` — GPIO number
- `sample_rate_hz` — modulator sample rate (e.g. 1000000 for 1 MHz)
- `out_sdm` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pin, zero sample rate, or NULL pointer.

---

### `blusys_sdm_close`

```c
blusys_err_t blusys_sdm_close(blusys_sdm_t *sdm);
```

Disables the SDM output and frees the handle.

---

### `blusys_sdm_set_density`

```c
blusys_err_t blusys_sdm_set_density(blusys_sdm_t *sdm, int8_t density);
```

Sets the pulse density, which controls the average output level after low-pass filtering.

**Parameters:**
- `sdm` — handle
- `density` — signed 8-bit value in range `[-128, 127]`

**Density scale:**

| `density` | Approximate output |
|-----------|--------------------|
| -128 | ~0% (always low) |
| 0 | ~50% |
| 127 | ~100% (always high) |

**Returns:** `BLUSYS_OK`.

## Lifecycle

1. `blusys_sdm_open()` — configure channel, output begins
2. `blusys_sdm_set_density()` — change output level as needed
3. `blusys_sdm_close()` — disable output and release

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_sdm_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the SDM module.

## Limitations

- one channel per handle; each handle occupies one GPIO and one hardware SDM channel slot
- the output is a high-frequency digital signal; an RC filter is required for analog use (a 4.7 kΩ + 100 nF filter is a common starting point)
- SDM is distinct from LEDC PWM: SDM uses fixed frequency with variable pulse density; LEDC uses configurable frequency with duty control

## Example App

See `examples/validation/hal_io_lab` (SDM scenario).
