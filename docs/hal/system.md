# System

Common runtime helpers: uptime, reset reason, heap statistics, and soft restart.

## At a glance

- available on all supported targets
- read-only helpers are thread-safe
- restart does not return

## Quick example

```c
uint64_t uptime_us;
if (blusys_system_uptime_us(&uptime_us) == BLUSYS_OK) {
    printf("uptime_us=%" PRIu64 "\n", uptime_us);
}
```

## Common mistakes

- passing NULL output pointers
- expecting uptime to survive restart or deep sleep
- expecting raw ESP-IDF reset reasons

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- read functions are safe to call concurrently
- restart terminates normal program flow

## Limitations

- uptime resets after restart or deep sleep wakeup
- unknown reset reasons map to `BLUSYS_RESET_REASON_UNKNOWN`

## Example app

`examples/validation/hal_io_lab` (`system_info`)
