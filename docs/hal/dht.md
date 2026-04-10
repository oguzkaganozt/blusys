# DHT

DHT11 / DHT22 (AM2302) one-wire temperature and humidity sensor driver using RMT for precise bit-level timing.

## Quick Example

```c
#include "blusys/blusys.h"
#include "blusys/drivers/sensor/dht.h"

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

Setting the wrong `type` in the config causes garbled readings because the two sensors encode data differently. DHT22 uses 16-bit scaled values; DHT11 uses integer + decimal bytes.

### Reading immediately after power-on

```c
/* WRONG — sensor not ready */
blusys_dht_open(&cfg, &dht);
blusys_dht_read(dht, &reading);

/* RIGHT — wait 1 s for stabilisation */
blusys_dht_open(&cfg, &dht);
vTaskDelay(pdMS_TO_TICKS(1000));
blusys_dht_read(dht, &reading);
```

## Target Support

| Target   | Supported |
|----------|-----------|
| ESP32    | Yes       |
| ESP32-C3 | Yes       |
| ESP32-S3 | Yes       |

All targets with RMT hardware are supported.

## Types

### `blusys_dht_t`

Opaque handle returned by `blusys_dht_open()`.

```c
typedef struct blusys_dht blusys_dht_t;
```

### `blusys_dht_type_t`

```c
typedef enum {
    BLUSYS_DHT_TYPE_DHT11,   /* 0–50 °C (±2 °C), 20–80 % RH (±5 %) */
    BLUSYS_DHT_TYPE_DHT22,   /* −40–80 °C (±0.5 °C), 0–100 % RH (±2 %) */
} blusys_dht_type_t;
```

### `blusys_dht_config_t`

```c
typedef struct {
    int               pin;   /* GPIO connected to DHT data line */
    blusys_dht_type_t type;  /* DHT11 or DHT22 */
} blusys_dht_config_t;
```

### `blusys_dht_reading_t`

```c
typedef struct {
    float temperature_c;   /* degrees Celsius */
    float humidity_pct;    /* relative humidity 0–100 % */
} blusys_dht_reading_t;
```

## Functions

### `blusys_dht_open`

```c
blusys_err_t blusys_dht_open(const blusys_dht_config_t *config,
                              blusys_dht_t **out_handle);
```

Open a DHT sensor and allocate RMT TX/RX channels on the given pin.

| Parameter    | Description                          |
|-------------|--------------------------------------|
| `config`     | Sensor configuration (pin + type)    |
| `out_handle` | Receives the handle on success       |

| Return                    | Condition                          |
|--------------------------|------------------------------------|
| `BLUSYS_OK`              | Success                            |
| `BLUSYS_ERR_INVALID_ARG` | NULL pointer or invalid GPIO       |
| `BLUSYS_ERR_NO_MEM`      | Allocation failed                  |
| `BLUSYS_ERR_INTERNAL`    | RMT channel setup failed           |

### `blusys_dht_close`

```c
blusys_err_t blusys_dht_close(blusys_dht_t *handle);
```

Release all resources associated with the handle.

| Parameter | Description                      |
|----------|----------------------------------|
| `handle`  | Handle returned by `open()`      |

| Return                    | Condition            |
|--------------------------|----------------------|
| `BLUSYS_OK`              | Success              |
| `BLUSYS_ERR_INVALID_ARG` | NULL handle          |

### `blusys_dht_read`

```c
blusys_err_t blusys_dht_read(blusys_dht_t *handle,
                              blusys_dht_reading_t *out_reading);
```

Read temperature and humidity from the sensor. If called within 2 seconds of the previous read, returns the cached result immediately.

| Parameter     | Description                            |
|--------------|----------------------------------------|
| `handle`      | Handle returned by `open()`            |
| `out_reading` | Receives temperature and humidity      |

| Return                    | Condition                                 |
|--------------------------|-------------------------------------------|
| `BLUSYS_OK`              | Success (or cached result returned)       |
| `BLUSYS_ERR_INVALID_ARG` | NULL pointer                              |
| `BLUSYS_ERR_TIMEOUT`     | Sensor did not respond                    |
| `BLUSYS_ERR_IO`          | Checksum mismatch or invalid frame        |
| `BLUSYS_ERR_INVALID_STATE` | Handle is closing or bus is busy        |

## Lifecycle

1. `blusys_dht_open()` — allocates handle and configures RMT channels
2. `blusys_dht_read()` — triggers a start pulse, captures the 40-bit response, decodes
3. `blusys_dht_close()` — releases RMT channels and frees memory

## Thread Safety

All functions are serialised by an internal mutex. The mutex is released before blocking on the RMT RX completion (task notification), so other threads will not deadlock — they will see `BLUSYS_ERR_INVALID_STATE` if a read is already in progress.

## Limitations

- An external **4.7 kΩ–10 kΩ pull-up resistor** to 3.3 V on the data pin is required; the internal pull-up is too weak.
- Minimum interval between reads is **2 seconds**. Faster calls return the previous cached reading.
- Only one DHT sensor per handle. Open a separate handle for each sensor.
- The 40-bit frame is validated with the DHT checksum (sum of first 4 bytes). A mismatch returns `BLUSYS_ERR_IO`.
- DHT sensors need **1 second** after power-on before the first read.

## Example App

See `examples/validation/dht_basic/` — reads temperature and humidity 10 times at 3-second intervals.

```bash
blusys build dht_basic
blusys run   dht_basic
```
