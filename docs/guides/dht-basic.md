# Read Temperature & Humidity with a DHT Sensor

## Problem Statement

You have a DHT11 or DHT22 (AM2302) sensor and want to read ambient temperature and relative humidity.

## Prerequisites

- **Board:** Any ESP32, ESP32-C3, or ESP32-S3 development board.
- **Sensor:** DHT11 or DHT22 / AM2302.
- **Wiring:**
    - DHT **VCC** → 3.3 V
    - DHT **GND** → GND
    - DHT **DATA** → GPIO (default: GPIO 4 on ESP32/S3, GPIO 3 on C3)
    - **4.7 kΩ pull-up resistor** between DATA and 3.3 V

## Minimal Example

```c
#include "blusys/blusys.h"
#include "blusys/sensor/dht.h"

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

## APIs Used

| Function            | Purpose                                     |
|--------------------|---------------------------------------------|
| `blusys_dht_open`  | Configure GPIO pin and sensor type          |
| `blusys_dht_read`  | Trigger measurement, decode 40-bit response |
| `blusys_dht_close` | Release RMT channels and free memory        |

## Expected Runtime Behavior

- The sensor needs **1 second** after power-on before the first read succeeds.
- Each read sends a 20 ms start pulse and captures 40 data bits via RMT.
- Reads within 2 seconds of the previous read return the **cached** result (no bus traffic).
- DHT22 is more accurate (±0.5 °C, ±2 % RH) than DHT11 (±2 °C, ±5 % RH).

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

## Example App

The [`examples/dht_basic/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/dht_basic) example reads 10 samples at 3-second intervals and prints each to the console.

```bash
blusys build dht_basic
blusys run   dht_basic
```

Override the pin or sensor type via menuconfig:

```bash
blusys config dht_basic
# → Blusys DHT Basic Example → DHT data GPIO pin / DHT sensor type
```

## API Reference

See [DHT API Reference](../modules/dht.md) for full type definitions, all return codes, and thread-safety details.
