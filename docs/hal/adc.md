# ADC

Single-channel ADC input: raw conversion values and calibrated millivolt readings.

## Quick Example

```c
#include "blusys/adc.h"

void app_main(void)
{
    blusys_adc_t *adc;
    int mv;

    if (blusys_adc_open(4, BLUSYS_ADC_ATTEN_DB_12, &adc) != BLUSYS_OK) {
        return;
    }

    if (blusys_adc_read_mv(adc, &mv) == BLUSYS_OK) {
        /* use mv */
    }

    blusys_adc_close(adc);
}
```

## Common Mistakes

- choosing a GPIO that is not ADC-capable on the selected target
- applying a voltage outside the safe range for the board
- assuming calibrated millivolt conversion is always available

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_adc_t`

```c
typedef struct blusys_adc blusys_adc_t;
```

Opaque handle returned by `blusys_adc_open()`.

### `blusys_adc_atten_t`

```c
typedef enum {
    BLUSYS_ADC_ATTEN_DB_0 = 0,   /* 0–750 mV */
    BLUSYS_ADC_ATTEN_DB_2_5,     /* 0–1050 mV */
    BLUSYS_ADC_ATTEN_DB_6,       /* 0–1300 mV */
    BLUSYS_ADC_ATTEN_DB_12,      /* 0–2500 mV */
} blusys_adc_atten_t;
```

Selects the input attenuation and the corresponding full-scale voltage range.

## Functions

### `blusys_adc_open`

```c
blusys_err_t blusys_adc_open(int pin, blusys_adc_atten_t atten, blusys_adc_t **out_adc);
```

Opens one ADC channel on the given GPIO pin. ADC unit and channel are resolved internally.

**Parameters:**
- `pin` — GPIO number; must be an ADC1-capable pin on the target
- `atten` — input attenuation and voltage range
- `out_adc` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pin or attenuation, `BLUSYS_ERR_BUSY` if the channel is already owned.

---

### `blusys_adc_close`

```c
blusys_err_t blusys_adc_close(blusys_adc_t *adc);
```

Releases the ADC channel and frees the handle.

---

### `blusys_adc_read_raw`

```c
blusys_err_t blusys_adc_read_raw(blusys_adc_t *adc, int *out_raw);
```

Reads the raw ADC conversion value. Range depends on the hardware resolution (typically 0–4095 for 12-bit).

**Parameters:**
- `adc` — handle
- `out_raw` — output pointer for the raw value

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_raw` is NULL.

---

### `blusys_adc_read_mv`

```c
blusys_err_t blusys_adc_read_mv(blusys_adc_t *adc, int *out_mv);
```

Reads a calibrated millivolt value. Requires factory calibration data to be stored in eFuse or OTP.

**Parameters:**
- `adc` — handle
- `out_mv` — output pointer for millivolt value

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_NOT_SUPPORTED` when calibrated conversion is not available on the current board, `BLUSYS_ERR_INVALID_ARG` if `out_mv` is NULL.

## Lifecycle

1. `blusys_adc_open()` — claim channel
2. `blusys_adc_read_raw()` / `blusys_adc_read_mv()` — one-shot reads
3. `blusys_adc_close()` — release channel

## Thread Safety

- concurrent reads on the same handle are serialized internally
- different handles may be used independently on different ADC1 channels
- the same channel cannot be opened by more than one handle at a time
- do not call `blusys_adc_close()` concurrently with other calls on the same handle

## Limitations

- limited to ADC1-backed GPIOs on the supported targets
- the public API is pin-based; ADC unit and channel IDs are not exposed
- reads are blocking and one-shot only
- ADC readings are approximate and may be noisy depending on board layout and source impedance
- `blusys_adc_read_mv()` is only valid for attenuation modes with calibration support; `BLUSYS_ADC_ATTEN_DB_0` may return `BLUSYS_ERR_NOT_SUPPORTED` on some boards

## Example App

See `examples/reference/hal` (ADC scenario).
