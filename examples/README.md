# Examples

The tree is intentionally small (see root `inventory.yml`). Everything else is selected via **menuconfig** scenarios inside merged projects.

## quickstart/

Fork-ready product templates. Start here.

- **handheld** — interactive controller (host + device); encoder-driven UI, storage + BLE provisioning
- **headless** — connected telemetry reference; WiFi, OTA, diagnostics, optional local OLED

## reference/

Focused capability demos. Pick a scenario in menuconfig.

- **hal** — HAL demos: GPIO, PWM, button, timer, NVS, ADC, SPI loopback, I2C scan, UART
- **display** — LCD bring-up, LVGL UI, encoder, OLED; SPI/I2C pin config per target
- **connectivity** — WiFi connect, HTTP client, MQTT client

## validation/

Internal CI / hardware smoke. Not the primary learning path.

- **hal_io_lab** — consolidated digital / sensor / timing checks
- **peripheral_lab** — I2C/SPI slave, TWAI, RMT, I2S, SD, GPIO expander, SSD1306
- **network_services** — HTTP server, WebSocket, mDNS, SNTP, OTA, WiFi provisioning, local control
- **platform_lab** — smoke, storage, power, console, framework core, concurrency suites
- **wireless_esp_lab** / **wireless_bt_lab** — RF stacks
- **usb_peripheral_lab** — USB host / device / HID
- **headless_telemetry_low_power** — low-power telemetry profile
- **connected_device** / **connected_headless** — regression coverage (superseded by quickstarts)
- **framework_device_basic** — device profile smoke build
