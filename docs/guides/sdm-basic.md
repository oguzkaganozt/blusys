# Produce A Pseudo-Analog Output With Sigma-Delta Modulation

## Problem Statement

You want a simple analog-like output on a GPIO pin without using the DAC peripheral — for example to control LED brightness with more resolution than PWM, or to drive a load through an RC filter.

## Prerequisites

- a supported board
- a GPIO pin with an RC low-pass filter on the output (e.g. 4.7 kΩ + 100 nF) if an analog voltage is needed
- or a direct connection if a variable duty-cycle bit stream is acceptable (e.g. driving an LED)

## Minimal Example

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

## APIs Used

- `blusys_sdm_open()` allocates an SDM channel on the pin and enables the output at the given sample rate
- `blusys_sdm_set_density()` sets the pulse density in `[-128, 127]`; higher values produce a longer high time
- `blusys_sdm_close()` releases the channel and stops the output

## Density-to-Voltage Mapping

After an RC low-pass filter the output voltage tracks approximately linearly with density:

| density | Output |
|---------|--------|
| -128 | ~0 V (GND) |
| 0 | ~VCC/2 (~1.65 V at 3.3 V) |
| 127 | ~VCC (~3.3 V) |

## Expected Runtime Behavior

- the pin toggles at the sample rate with a bit pattern whose density reflects the configured value
- `set_density()` takes effect immediately on the next sample clock cycle
- no ISR or callback is involved; the SDM peripheral runs autonomously

## Common Mistakes

- omitting the RC filter when a smooth voltage is needed; without it, the output is a high-speed digital signal
- confusing SDM with LEDC PWM: both produce variable duty-cycle signals, but SDM uses a fixed frequency and is better suited for audio or high-resolution DC approximation

## Example App

See `examples/sdm_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [SDM API Reference](../modules/sdm.md).
