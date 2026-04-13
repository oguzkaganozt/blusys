# Headless LAN control (reference)

Headless **`blusys::app`** reference with **connectivity** + **`lan_control`** only — the same capability pairing as [`lan_control_basic`](../validation/lan_control_basic/README.md), kept as a **public reference** for operators who want a minimal LAN HTTP/mDNS surface without the full headless telemetry quickstart stack.

## Build

```bash
cd examples/reference/headless_lan_control
idf.py set-target esp32
idf.py build
```

Configure Wi-Fi via `idf.py menuconfig` → **Headless LAN control reference**.

No host SDL target; use hardware or QEMU.
