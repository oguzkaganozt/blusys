# Temperature Sensor

On-chip die temperature sensor. Useful for thermal monitoring and derating decisions. Not an ambient temperature measurement.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Temperature Sensor Basics](../guides/temp-sensor-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | no |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

On the original ESP32, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_TEMP_SENSOR)` to check at runtime.

## Types

### `blusys_temp_sensor_t`

```c
typedef struct blusys_temp_sensor blusys_temp_sensor_t;
```

Opaque handle returned by `blusys_temp_sensor_open()`.

## Functions

### `blusys_temp_sensor_open`

```c
blusys_err_t blusys_temp_sensor_open(blusys_temp_sensor_t **out_tsens);
```

Enables and calibrates the on-chip temperature sensor.

**Parameters:**
- `out_tsens` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_tsens` is NULL, `BLUSYS_ERR_NOT_SUPPORTED` on the original ESP32, translated ESP-IDF error on driver initialization failure.

---

### `blusys_temp_sensor_close`

```c
blusys_err_t blusys_temp_sensor_close(blusys_temp_sensor_t *tsens);
```

Disables the sensor and frees the handle.

---

### `blusys_temp_sensor_read_celsius`

```c
blusys_err_t blusys_temp_sensor_read_celsius(blusys_temp_sensor_t *tsens, float *out_celsius);
```

Reads the current die temperature in degrees Celsius.

**Parameters:**
- `tsens` — handle
- `out_celsius` — output pointer for the temperature value

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_celsius` is NULL.

## Lifecycle

1. `blusys_temp_sensor_open()` — enable and calibrate
2. `blusys_temp_sensor_read_celsius()` — read temperature as needed
3. `blusys_temp_sensor_close()` — disable and release

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_temp_sensor_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the temperature sensor module.

## Limitations

- reads silicon die temperature, not ambient temperature
- typical accuracy is ±2 °C after calibration; do not rely on single readings for safety-critical decisions

## Example App

See `examples/temp_sensor_basic/`.
