# Handheld Bluetooth (reference)

Compact **handheld** interactive reference: ST7735-class profile, **storage** (NVS init) + **`bluetooth_capability`** in GAP-only mode (no custom GATT services). Demonstrates NimBLE bring-up and capability events on the canonical interactive path.

Host SDL: `blusys host-build examples/reference/handheld_bluetooth` (Bluetooth is a stub on host; UI and reducer still run).

## Device build

```bash
cd examples/reference/handheld_bluetooth
idf.py set-target esp32c3
idf.py build
```
