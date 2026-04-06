# Read The On-Chip Temperature

## Problem Statement

You want to monitor the ESP32-C3 or ESP32-S3 die temperature to detect thermal stress or log operating conditions.

## Prerequisites

- a supported board (ESP32-C3 or ESP32-S3)
- no external wiring required — the sensor is on-chip

## Minimal Example

```c
blusys_temp_sensor_t *tsens;
float celsius;

blusys_temp_sensor_open(&tsens);
blusys_temp_sensor_read_celsius(tsens, &celsius);
printf("die temp: %.1f C\n", celsius);
blusys_temp_sensor_close(tsens);
```

## APIs Used

- `blusys_temp_sensor_open()` enables the on-chip sensor and applies factory calibration
- `blusys_temp_sensor_read_celsius()` triggers a single conversion and returns the result
- `blusys_temp_sensor_close()` disables the sensor and releases the handle

## Expected Runtime Behavior

- readings reflect silicon die temperature, not ambient temperature
- the die runs warmer than ambient due to self-heating from active peripherals and the CPU
- typical accuracy is ±2 °C; readings are suitable for trend monitoring, not precision measurement

## Common Mistakes

- using this module on the original ESP32 (no on-chip temperature sensor; returns `BLUSYS_ERR_NOT_SUPPORTED`)
- treating a single reading as ambient temperature — the die is always warmer than the surrounding air when running

## Example App

See `examples/temp_sensor_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [Temperature Sensor API Reference](../modules/temp_sensor.md).
