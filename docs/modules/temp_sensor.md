# Temperature Sensor

## Purpose

The `temp_sensor` module reads the on-chip temperature sensor:

- open the temperature sensor
- read the current die temperature in degrees Celsius
- close the handle

This is the internal silicon temperature, not an ambient measurement. It is useful for thermal monitoring and derating decisions.

## Supported Targets

- ESP32-C3
- ESP32-S3

Not available on the original ESP32 (no on-chip temperature sensor peripheral).

## Quick Start Example

```c
#include <stdio.h>

#include "blusys/temp_sensor.h"

void app_main(void)
{
    blusys_temp_sensor_t *tsens;
    float celsius;

    if (blusys_temp_sensor_open(&tsens) != BLUSYS_OK) {
        return;
    }

    blusys_temp_sensor_read_celsius(tsens, &celsius);
    printf("die temp: %.1f C\n", celsius);
    blusys_temp_sensor_close(tsens);
}
```

## Lifecycle Model

Temperature sensor is handle-based:

1. call `blusys_temp_sensor_open()` — enables and calibrates the sensor
2. call `blusys_temp_sensor_read_celsius()` as needed
3. call `blusys_temp_sensor_close()` when finished

## Blocking APIs

- `blusys_temp_sensor_open()`
- `blusys_temp_sensor_close()`
- `blusys_temp_sensor_read_celsius()`

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_temp_sensor_close()` concurrently with other calls using the same handle

## ISR Notes

No ISR-safe calls are defined for the temperature sensor module.

## Limitations

- reads silicon die temperature, not ambient temperature
- typical accuracy is ±2 °C after calibration; do not rely on single readings for safety-critical decisions
- available only on targets with the temperature sensor peripheral: ESP32-C3 and ESP32-S3

## Error Behavior

- null pointer arguments return `BLUSYS_ERR_INVALID_ARG`
- `BLUSYS_ERR_NOT_SUPPORTED` is returned on the original ESP32
- driver initialization failures return the translated ESP-IDF error

## Example App

See `examples/temp_sensor_basic/`.
