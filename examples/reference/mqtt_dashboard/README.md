# MQTT dashboard (reference)

Interactive LVGL shell with **real MQTT** on the **host (SDL)** using `mqtt_host_capability` (libmosquitto). Buttons publish JSON to `blusys/demo/cmd`; the client subscribes to `blusys/demo/#` so you can see traffic with any MQTT client.

**ESP-IDF** builds the same UI; MQTT on device is not wired in this example (use `mqtt_basic` or extend with `blusys_mqtt_*` + Wi‑Fi).

## Host (SDL)

Install **libmosquitto** (e.g. `sudo apt install libmosquitto-dev`), then:

```bash
blusys host-build examples/reference/mqtt_dashboard
```

Run `build-host/mqtt_dashboard_host`. Optional CMake cache: `BLUSYS_MQTT_DASH_BROKER` (default `mqtt://test.mosquitto.org`), `BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE` (0 = 320×240, 1 = 480×320).

Public test brokers are for demos only—do not send secrets.

## Device

```bash
blusys build examples/reference/mqtt_dashboard esp32s3
```

Display profile: `menuconfig` → **MQTT Dashboard (display)**.
