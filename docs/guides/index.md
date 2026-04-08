# Task Guides

Each guide walks through one specific use case: problem statement, wiring prerequisites, a minimal runnable example, and common mistakes to avoid.

## HAL

Direct hardware access — use these when you need low-level control over ESP32 peripherals.

<div class="grid cards" markdown>

-   :material-cable-data:{ .lg .middle } **I/O & Communication**

    ---

    GPIO, serial buses, and communication protocols.

    [:octicons-arrow-right-24: GPIO](gpio-basic.md) · [UART (Blocking)](uart-loopback.md) · [UART (Async)](uart-async.md) · [I2C Master](i2c-scan.md) · [I2C Slave](i2c-slave-basic.md) · [SPI Master](spi-loopback.md) · [SPI Slave](spi-slave-basic.md) · [TWAI](twai-basic.md) · [I2S TX](i2s-basic.md) · [I2S RX](i2s-rx-basic.md) · [1-Wire](one-wire-basic.md) · [GPIO Expander](gpio-expander-basic.md) · [Touch](touch-basic.md) · [RMT TX](rmt-basic.md) · [RMT RX](rmt-rx-basic.md)

-   :material-sine-wave:{ .lg .middle } **Analog**

    ---

    ADC reads, DAC output, sigma-delta modulation, and PWM.

    [:octicons-arrow-right-24: ADC](adc-basic.md) · [DAC](dac-basic.md) · [SDM](sdm-basic.md) · [PWM](pwm-basic.md)

-   :material-timer-outline:{ .lg .middle } **Timers & Counters**

    ---

    General-purpose timers, pulse counting, and motor PWM.

    [:octicons-arrow-right-24: Timer](timer-basic.md) · [PCNT](pcnt-basic.md) · [MCPWM](mcpwm-basic.md)

-   :material-database:{ .lg .middle } **Storage**

    ---

    Non-volatile key-value storage and SD card access.

    [:octicons-arrow-right-24: NVS](nvs-basic.md) · [SD/MMC](sdmmc-basic.md) · [SD over SPI](sd-spi-basic.md)

-   :material-chip:{ .lg .middle } **Device**

    ---

    Chip info, sleep modes, watchdog, and on-chip temperature.

    [:octicons-arrow-right-24: System Info](system-info.md) · [Sleep](sleep-basic.md) · [Watchdog](wdt-basic.md) · [Temperature](temp-sensor-basic.md) · [eFuse](efuse-basic.md) · [USB Host](usb-host-basic.md) · [USB Device](usb-device-basic.md)

</div>

## Services

Application building blocks — use these for common tasks so you can focus on your business logic.

<div class="grid cards" markdown>

-   :material-monitor:{ .lg .middle } **Display**

    ---

    LCD panels, addressable LED strips, and 7-segment displays.

    [:octicons-arrow-right-24: LCD](lcd-basic.md) · [LED Strip](led-strip-basic.md) · [7-Segment](seven-seg-basic.md) · [UI (LVGL)](ui-basic.md)

-   :material-gesture-tap-button:{ .lg .middle } **Input**

    ---

    Debounced buttons and rotary encoders.

    [:octicons-arrow-right-24: Button](button-basic.md) · [Encoder](encoder-basic.md) · [USB HID](usb-hid-basic.md)

-   :material-thermometer:{ .lg .middle } **Sensor**

    ---

    Temperature, humidity, and environmental sensors.

    [:octicons-arrow-right-24: DHT](dht-basic.md)

-   :material-volume-high:{ .lg .middle } **Actuator**

    ---

    Buzzers, speakers, and motor drivers.

    [:octicons-arrow-right-24: Buzzer](buzzer-basic.md)

-   :material-wifi:{ .lg .middle } **Connectivity**

    ---

    WiFi, Bluetooth, BLE, ESP-NOW, and network discovery.

    [:octicons-arrow-right-24: WiFi](wifi-connect.md) · [WiFi Provisioning](wifi-prov-basic.md) · [ESP-NOW](espnow-basic.md) · [Bluetooth](bluetooth-basic.md) · [BLE GATT](ble_gatt-basic.md) · [mDNS](mdns-basic.md)

-   :material-swap-vertical:{ .lg .middle } **Protocol**

    ---

    HTTP, MQTT, and WebSocket application protocols.

    [:octicons-arrow-right-24: HTTP Client](http-basic.md) · [HTTP Server](http-server-basic.md) · [MQTT](mqtt-basic.md) · [WebSocket Client](ws-client-basic.md)

-   :material-cog:{ .lg .middle } **System**

    ---

    Console, filesystem, power management, time sync, and OTA updates.

    [:octicons-arrow-right-24: Console](console-basic.md) · [Filesystem](fs-basic.md) · [FAT Filesystem](fatfs-basic.md) · [Power Management](power-mgmt-basic.md) · [SNTP](sntp-basic.md) · [OTA](ota-basic.md) · [Local Control](local-ctrl-basic.md)

</div>

## Testing

<div class="grid cards" markdown>

-   :material-test-tube:{ .lg .middle } **Testing**

    ---

    Hardware smoke tests, concurrency stress tests, and validation report templates.

    [:octicons-arrow-right-24: Smoke Tests](hardware-smoke-tests.md) · [Concurrency](concurrency-tests.md) · [Validation Template](hardware-validation-report-template.md)

</div>
