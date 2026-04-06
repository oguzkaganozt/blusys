# Task Guides

Each guide walks through one specific use case: problem statement, wiring prerequisites, a minimal runnable example, and common mistakes to avoid.

Pick a peripheral category to get started.

<div class="grid cards" markdown>

-   :material-chip:{ .lg .middle } **Core Peripherals**

    ---

    GPIO, UART, I2C, SPI — the building blocks of embedded communication.

    [:octicons-arrow-right-24: GPIO](gpio-basic.md) · [UART (Blocking)](uart-loopback.md) · [UART (Async)](uart-async.md) · [I2C Master](i2c-scan.md) · [I2C Slave](i2c-slave-basic.md) · [SPI Master](spi-loopback.md) · [SPI Slave](spi-slave-basic.md)

-   :material-sine-wave:{ .lg .middle } **Analog**

    ---

    ADC reads, DAC output, sigma-delta modulation, and PWM.

    [:octicons-arrow-right-24: ADC](adc-basic.md) · [DAC](dac-basic.md) · [SDM](sdm-basic.md) · [PWM](pwm-basic.md)

-   :material-timer-outline:{ .lg .middle } **Timers & Counters**

    ---

    General-purpose timers, pulse counting, RMT pulse sequences, and motor PWM.

    [:octicons-arrow-right-24: Timer](timer-basic.md) · [PCNT](pcnt-basic.md) · [RMT TX](rmt-basic.md) · [RMT RX](rmt-rx-basic.md) · [MCPWM](mcpwm-basic.md)

-   :material-transit-connection-variant:{ .lg .middle } **Bus**

    ---

    TWAI/CAN frames, I2S audio, and SD card access over SDMMC or SPI.

    [:octicons-arrow-right-24: TWAI](twai-basic.md) · [I2S TX](i2s-basic.md) · [I2S RX](i2s-rx-basic.md) · [SD/MMC](sdmmc-basic.md) · [SD over SPI](sd-spi-basic.md)

-   :material-thermometer:{ .lg .middle } **Sensors**

    ---

    Capacitive touch and on-chip die temperature.

    [:octicons-arrow-right-24: Touch](touch-basic.md) · [Temperature](temp-sensor-basic.md)

-   :material-cog:{ .lg .middle } **System**

    ---

    Runtime info, interactive console, persistent storage, sleep modes, and watchdog.

    [:octicons-arrow-right-24: System Info](system-info.md) · [Console](console-basic.md) · [NVS](nvs-basic.md) · [Filesystem](fs-basic.md) · [FAT Filesystem](fatfs-basic.md) · [Sleep](sleep-basic.md) · [Watchdog](wdt-basic.md)

-   :material-wifi:{ .lg .middle } **Networking**

    ---

    WiFi, HTTP, MQTT, WebSocket, OTA updates, SNTP time sync, mDNS discovery, BLE, BLE GATT, and ESP-NOW.

    [:octicons-arrow-right-24: WiFi](wifi-connect.md) · [WiFi Provisioning](wifi-prov-basic.md) · [HTTP Client](http-basic.md) · [HTTP Server](http-server-basic.md) · [MQTT](mqtt-basic.md) · [WebSocket Client](ws-client-basic.md) · [OTA](ota-basic.md) · [SNTP](sntp-basic.md) · [mDNS](mdns-basic.md) · [Bluetooth](bluetooth-basic.md) · [BLE GATT](ble_gatt-basic.md) · [ESP-NOW](espnow-basic.md)

-   :material-test-tube:{ .lg .middle } **Testing**

    ---

    Hardware smoke tests, concurrency stress tests, and validation report templates.

    [:octicons-arrow-right-24: Smoke Tests](hardware-smoke-tests.md) · [Concurrency](concurrency-tests.md) · [Validation Template](hardware-validation-report-template.md)

</div>
