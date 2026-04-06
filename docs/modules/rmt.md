# RMT

Timed digital pulse sequences: TX for generating IR codes and similar protocols, RX for capturing pulses.

!!! tip "Task Guides"
    For step-by-step walkthroughs, see [RMT TX](../guides/rmt-basic.md) or [RMT RX](../guides/rmt-rx-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_rmt_pulse_t`

```c
typedef struct {
    bool level;          /* signal level: true = high, false = low */
    uint32_t duration_us; /* pulse duration in microseconds; must be > 0 */
} blusys_rmt_pulse_t;
```

Shared by both TX and RX. The internal resolution is 1 MHz (1 µs per tick).

### `blusys_rmt_t`

```c
typedef struct blusys_rmt blusys_rmt_t;
```

Opaque TX handle returned by `blusys_rmt_open()`.

### `blusys_rmt_rx_t`

```c
typedef struct blusys_rmt_rx blusys_rmt_rx_t;
```

Opaque RX handle returned by `blusys_rmt_rx_open()`.

### `blusys_rmt_rx_config_t`

```c
typedef struct {
    uint32_t signal_range_min_ns;  /* glitch filter: pulses shorter than this are ignored */
    uint32_t signal_range_max_ns;  /* idle threshold: silence longer than this ends the frame */
} blusys_rmt_rx_config_t;
```

## Functions — TX

### `blusys_rmt_open`

```c
blusys_err_t blusys_rmt_open(int pin, bool idle_level, blusys_rmt_t **out_rmt);
```

Opens an RMT TX channel on the given pin.

**Parameters:**
- `pin` — GPIO number
- `idle_level` — signal level when idle (between transmissions)
- `out_rmt` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.

---

### `blusys_rmt_close`

```c
blusys_err_t blusys_rmt_close(blusys_rmt_t *rmt);
```

Releases the RMT TX channel and frees the handle.

---

### `blusys_rmt_write`

```c
blusys_err_t blusys_rmt_write(blusys_rmt_t *rmt,
                              const blusys_rmt_pulse_t *pulses,
                              size_t pulse_count,
                              int timeout_ms);
```

Sends a pulse sequence. Blocks until the full sequence is transmitted or the timeout expires.

**Parameters:**
- `rmt` — TX handle
- `pulses` — array of pulses to send
- `pulse_count` — number of pulses; must be greater than zero
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for empty pulse list or zero durations, `BLUSYS_ERR_TIMEOUT` on transmission timeout.

## Functions — RX

### `blusys_rmt_rx_open`

```c
blusys_err_t blusys_rmt_rx_open(int pin,
                                 const blusys_rmt_rx_config_t *config,
                                 blusys_rmt_rx_t **out_rmt_rx);
```

Opens an RMT RX channel on the given pin.

**Parameters:**
- `pin` — GPIO number
- `config` — glitch filter and idle threshold settings
- `out_rmt_rx` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.

---

### `blusys_rmt_rx_close`

```c
blusys_err_t blusys_rmt_rx_close(blusys_rmt_rx_t *rmt_rx);
```

Releases the RMT RX channel and frees the handle.

---

### `blusys_rmt_rx_read`

```c
blusys_err_t blusys_rmt_rx_read(blusys_rmt_rx_t *rmt_rx,
                                 blusys_rmt_pulse_t *pulses,
                                 size_t max_pulses,
                                 size_t *out_count,
                                 int timeout_ms);
```

Waits for a pulse frame to complete (idle detected) and fills `pulses` with the captured sequence.

**Parameters:**
- `rmt_rx` — RX handle
- `pulses` — buffer for captured pulses
- `max_pulses` — capacity of the buffer
- `out_count` — number of pulses captured
- `timeout_ms` — milliseconds to wait; use `BLUSYS_TIMEOUT_FOREVER` to block indefinitely

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT` if no frame completes before the timeout.

## Lifecycle

**TX:** `blusys_rmt_open()` → `blusys_rmt_write()` → `blusys_rmt_close()`

**RX:** `blusys_rmt_rx_open()` → `blusys_rmt_rx_read()` → `blusys_rmt_rx_close()`

## Thread Safety

- concurrent calls on the same handle are serialized internally
- TX and RX handles are independent
- do not call close concurrently with other calls on the same handle

## Limitations

- pulse resolution is fixed at 1 MHz (1 µs per tick)
- no carrier modulation, protocol helpers, or async transmit
- one input/output pin per handle

## Example App

See `examples/rmt_basic/` for TX.
See `examples/rmt_rx_basic/` for RX.
