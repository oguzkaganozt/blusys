# Capture IR Remote Pulses With RMT RX

## Problem Statement

You want to capture the pulse sequence from an IR remote control (or any similar timed signal) and inspect the individual pulse durations and levels.

## Prerequisites

- a supported board
- an IR receiver module (e.g. VS1838B, TSOP38238) connected to a GPIO pin
- an IR remote control to point at the receiver

## Minimal Example

```c
blusys_rmt_rx_t *rmt_rx;
blusys_rmt_pulse_t pulses[256];
size_t count;

blusys_rmt_rx_config_t config = {
    .signal_range_min_ns = 1250,       /* ignore glitches < 1.25 µs */
    .signal_range_max_ns = 12000000,   /* end frame after 12 ms silence */
};

blusys_rmt_rx_open(4, &config, &rmt_rx);

blusys_rmt_rx_read(rmt_rx, pulses, 256, &count, 30000);

for (size_t i = 0; i < count; i++) {
    printf("[%u] level=%u duration=%u us\n",
           (unsigned)i, pulses[i].level, pulses[i].duration_us);
}

blusys_rmt_rx_close(rmt_rx);
```

## APIs Used

- `blusys_rmt_rx_open()` opens the RMT RX channel on a pin and configures glitch filter and idle threshold
- `blusys_rmt_rx_read()` blocks until a complete frame is received or the timeout expires, then fills the pulse array
- `blusys_rmt_rx_close()` releases the channel

## Config Parameters

| Parameter | Effect |
|-----------|--------|
| `signal_range_min_ns` | Glitch filter: pulses shorter than this are discarded. 1250 ns (1.25 µs) works well for NEC/RC5 IR. |
| `signal_range_max_ns` | Idle threshold: a gap longer than this marks the end of a frame. 12 ms covers most IR protocols. |

## Pulse Format

Each `blusys_rmt_pulse_t` in the output array represents one continuous period at a fixed level:

```c
typedef struct {
    bool     level;        /* true = high, false = low */
    uint32_t duration_us;  /* duration in microseconds */
} blusys_rmt_pulse_t;
```

A typical NEC IR frame produces ~68 pulses (9 ms burst, 4.5 ms space, 32 data bits × 2 pulses each).

## Expected Runtime Behavior

- `read()` arms the channel and blocks until a frame arrives or `timeout_ms` expires
- on a successful capture, `*out_count` contains the number of pulses filled into the array
- a timeout returns `BLUSYS_ERR_TIMEOUT`; the array contents are undefined
- consecutive calls to `read()` each capture one independent frame

## Common Mistakes

- setting `signal_range_min_ns = 0` — the RMT driver requires a non-zero glitch filter; `open()` rejects zero values
- using a bare photodiode instead of a demodulating IR receiver module — the raw IR carrier (~38 kHz) will flood the buffer with noise
- providing a `max_pulses` buffer too small for the protocol — NEC needs at least 68 entries; RC5 needs at least 28

## Example App

See `examples/rmt_rx_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.
