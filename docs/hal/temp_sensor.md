# Temperature Sensor

On-chip die temperature sensor. Useful for thermal monitoring and derating decisions. Not an ambient temperature measurement.

## Quick Example

```c
blusys_temp_sensor_t *tsens;
float celsius;

blusys_temp_sensor_open(&tsens);
blusys_temp_sensor_read_celsius(tsens, &celsius);
printf("die temp: %.1f C\n", celsius);
blusys_temp_sensor_close(tsens);
```

## Common Mistakes

- using this module on the original ESP32 (no on-chip temperature sensor; returns `BLUSYS_ERR_NOT_SUPPORTED`)
- treating a single reading as ambient temperature — the die is always warmer than the surrounding air when running

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | no |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

On the original ESP32, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_TEMP_SENSOR)` to check at runtime.

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_temp_sensor_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the temperature sensor module.

## Limitations

- reads silicon die temperature, not ambient temperature
- typical accuracy is ±2 °C after calibration; do not rely on single readings for safety-critical decisions

## Example App

See `examples/validation/hal_io_lab` (temp sensor scenario).
