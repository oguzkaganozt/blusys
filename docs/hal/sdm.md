# Sigma-Delta Modulation

High-frequency sigma-delta bitstream output on a GPIO. Useful for LED dimming, simple DAC approximation with an RC filter, or any load that accepts a PWM-style bitstream.

Density scale: `-128` ≈ 0% (always low), `0` ≈ 50%, `127` ≈ 100% (always high).

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

**ESP32, ESP32-C3, ESP32-S3** — all supported.

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
