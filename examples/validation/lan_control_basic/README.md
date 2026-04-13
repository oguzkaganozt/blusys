# LAN control basic (validation)

Minimal `blusys::app` firmware that links **connectivity** and **lan_control** only. Proves the LAN control capability wrapper and its integration event path on device builds.

## Build

```bash
cd examples/validation/lan_control_basic
idf.py set-target esp32
idf.py build
```

Configure Wi-Fi via `idf.py menuconfig` → **LAN control basic validation**.

There is no host SDL target; use a hardware or QEMU device build.
