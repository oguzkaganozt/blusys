# Power management

CPU and APB frequency scaling with automatic light sleep.

## At a glance

- `CONFIG_PM_ENABLE=y` is required
- locks raise or hold max CPU frequency
- idle intervals can drop toward the minimum frequency

## Quick example

```c
blusys_pm_config_t cfg = {
    .max_freq_mhz = 160,
    .min_freq_mhz = 40,
    .light_sleep_enable = true,
};
blusys_pm_configure(&cfg);
```

## Common mistakes

- missing `CONFIG_PM_ENABLE`
- forgetting acquire/release pairs
- stack-allocating the lock name
- enabling light sleep with an active UART

## Target support

| Target | Supported |
|--------|-----------|
| ESP32 | yes - requires `CONFIG_PM_ENABLE=y` |
| ESP32-C3 | yes - requires `CONFIG_PM_ENABLE=y` |
| ESP32-S3 | yes - requires `CONFIG_PM_ENABLE=y` |

## Thread safety

- configure/get are safe from any task
- acquire/release are safe from any task
- create/delete should be owned by one task

## ISR notes

- acquire/release are ISR-safe
- other functions are not

## Limitations

- valid frequencies are chip-specific
- PM locks are reference-counted

## Example app

`examples/validation/platform_lab/` (`plat_power_mgmt`)
