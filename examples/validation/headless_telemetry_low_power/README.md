# Headless telemetry + low power (validation)

Minimal headless firmware that enables **power management** (`CONFIG_PM_ENABLE=y`, matching scaffold **`low_power`** policy) alongside **connectivity** and **telemetry**. Used to ensure the policy overlay still links and runs with the connected headless stack.

## Build

```bash
cd examples/validation/headless_telemetry_low_power
idf.py set-target esp32c3
idf.py build
```

Configure Wi-Fi via `idf.py menuconfig` → **Headless telemetry + low power validation**.

There is no host SDL target.
