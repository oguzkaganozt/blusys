# DHT

DHT11 / DHT22 (AM2302) one-wire temperature and humidity sensor driver using RMT for precise bit-level timing.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    const blusys_dht_config_t cfg = {
        .pin  = 4,
        .type = BLUSYS_DHT_TYPE_DHT22,
    };

    blusys_dht_t *dht = NULL;
    blusys_dht_open(&cfg, &dht);

    /* Wait for sensor power-on stabilisation */
    vTaskDelay(pdMS_TO_TICKS(1000));

    blusys_dht_reading_t reading;
    blusys_err_t err = blusys_dht_read(dht, &reading);
    if (err == BLUSYS_OK) {
        printf("%.1f C  %.1f %%RH\n",
               reading.temperature_c, reading.humidity_pct);
    }

    blusys_dht_close(dht);
}
```

## Common Mistakes

### Missing pull-up resistor

The DHT data line is open-drain. Without a 4.7 kΩ external pull-up to 3.3 V the bus floats and reads will time out or return garbage.

### Reading too fast

```c
/* WRONG — 500 ms is below the 2 s minimum */
while (1) {
    blusys_dht_read(dht, &reading);
    vTaskDelay(pdMS_TO_TICKS(500));
}

/* RIGHT — 3 s gives the sensor time to sample */
while (1) {
    blusys_dht_read(dht, &reading);
    vTaskDelay(pdMS_TO_TICKS(3000));
}
```

Note: calling faster than 2 s won't cause an error — the driver returns the cached reading — but you won't get fresh data.

### Confusing DHT11 and DHT22 types

Setting the wrong `type` causes garbled readings because the two sensors encode data differently. DHT22 uses 16-bit scaled values; DHT11 uses integer + decimal bytes.

### Reading immediately after power-on

DHT sensors need **1 second** after power-on before the first read.

## Target Support

| Target   | Supported |
|----------|-----------|
| ESP32    | yes       |
| ESP32-C3 | yes       |
| ESP32-S3 | yes       |

All targets with RMT hardware are supported.

## Thread Safety

- all functions are serialised by an internal mutex
- the mutex is released before blocking on RMT RX completion, so other threads will not deadlock — they will see `BLUSYS_ERR_INVALID_STATE` if a read is already in progress

## ISR Notes

No ISR-safe calls are defined for the DHT module.

## Limitations

- an external **4.7 kΩ–10 kΩ pull-up resistor** to 3.3 V on the data pin is required; the internal pull-up is too weak
- minimum interval between reads is **2 seconds**; faster calls return the previous cached reading
- only one DHT sensor per handle — open a separate handle for each sensor
- the 40-bit frame is validated with the DHT checksum (sum of the first 4 bytes); a mismatch returns `BLUSYS_ERR_IO`

## Example App

See `examples/validation/hal_io_lab` (DHT scenario).
