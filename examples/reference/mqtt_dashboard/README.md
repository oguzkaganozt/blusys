# MQTT dashboard (reference)

Interactive LVGL shell with **real MQTT**:

- **Host (SDL):** `mqtt_host_capability` (libmosquitto). Buttons publish JSON to `blusys/demo/cmd`; the client subscribes to `blusys/demo/#`.
- **Device (ESP-IDF):** `connectivity_capability` (Wi‑Fi) + **`mqtt_esp_capability`** (`blusys_mqtt` / ESP-IDF MQTT client). Same topics and UI actions as the host build.

## Host (SDL)

Install **libmosquitto** (e.g. `sudo apt install libmosquitto-dev`), then:

```bash
blusys host-build examples/reference/mqtt_dashboard
```

Run `build-host/mqtt_dashboard_host`. Optional CMake cache: `BLUSYS_MQTT_DASH_BROKER` (default `mqtt://test.mosquitto.org`), `BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE` (0 = 320×240, 1 = 480×320).

Public test brokers are for demos only—do not send secrets.

## Device (Wi‑Fi + OLED)

1. Set **Wi‑Fi** and optional **broker** in `menuconfig` → **MQTT dashboard (device)** (`main/Kconfig.projbuild` defaults: `mqtt://test.mosquitto.org`).
2. **Display:** `menuconfig` → **Blusys framework → Display profiles → Dashboard-class display (device)** — or use chip defaults:
   - **`sdkconfig.defaults.esp32`** — SSD1306 128×64, I2C **SDA=21 / SCL=22** (classic ESP32).
   - **`sdkconfig.defaults.esp32c3`** — SSD1306; firmware sets **SDA=GPIO8 / SCL=GPIO9** in `main/integration/app_main.cpp` (`dashboard_device_profile`).
   - **`sdkconfig.defaults.esp32s3`** — SSD1306; **SDA=GPIO1 / SCL=GPIO2** (matches `profiles/ssd1306.hpp` and `dashboard_device_profile`).

```bash
blusys build examples/reference/mqtt_dashboard esp32c3
blusys flash examples/reference/mqtt_dashboard /dev/ttyACM0 esp32c3
```

**ESP32-S3 + SSD1306 on GPIO 1 / 2:**

```bash
blusys build examples/reference/mqtt_dashboard esp32s3
blusys flash examples/reference/mqtt_dashboard /dev/ttyUSB0 esp32s3
```

After Wi‑Fi gets an IP, the app connects to the broker, subscribes to `blusys/demo/#`, and the Live tab shows **tx/rx** counts and last message.

For **SPI TFT** on ESP32-S3 (ILI9341 / ILI9488 / ST7735), pick the profile in **Dashboard-class display (device)** instead of SSD1306 — same `blusys build … esp32s3` flow.
