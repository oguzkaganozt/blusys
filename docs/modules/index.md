# API Reference

Every Blusys module is documented with the same structure: **Types** (config structs and enums), **Functions** (full signatures, parameters, and return values), **Lifecycle**, **Thread Safety**, **Limitations**, and an **Example App** pointer.

## HAL

Direct hardware abstraction — thin wrappers over ESP-IDF drivers.

<div class="grid cards" markdown>

-   :material-cable-data:{ .lg .middle } **I/O & Communication**

    ---

    [:octicons-arrow-right-24: GPIO](gpio.md) · [UART](uart.md) · [I2C Master](i2c.md) · [I2C Slave](i2c_slave.md) · [SPI Master](spi.md) · [SPI Slave](spi_slave.md) · [TWAI](twai.md) · [I2S](i2s.md) · [1-Wire](one_wire.md) · [GPIO Expander](gpio_expander.md) · [Touch](touch.md) · [RMT](rmt.md)

-   :material-sine-wave:{ .lg .middle } **Analog**

    ---

    [:octicons-arrow-right-24: ADC](adc.md) · [DAC](dac.md) · [SDM](sdm.md) · [PWM](pwm.md)

-   :material-timer-outline:{ .lg .middle } **Timers & Counters**

    ---

    [:octicons-arrow-right-24: Timer](timer.md) · [PCNT](pcnt.md) · [MCPWM](mcpwm.md)

-   :material-database:{ .lg .middle } **Storage**

    ---

    [:octicons-arrow-right-24: NVS](nvs.md) · [SD/MMC](sdmmc.md) · [SD over SPI](sd_spi.md)

-   :material-chip:{ .lg .middle } **Device**

    ---

    [:octicons-arrow-right-24: System](system.md) · [Sleep](sleep.md) · [Watchdog](wdt.md) · [Temperature](temp_sensor.md) · [eFuse](efuse.md) · [ULP](ulp.md) · [USB Host](usb_host.md) · [USB Device](usb_device.md)

</div>

## Services

Application building blocks — higher-level modules for common tasks.

<div class="grid cards" markdown>

-   :material-monitor:{ .lg .middle } **Display**

    ---

    [:octicons-arrow-right-24: LCD](lcd.md) · [LED Strip](led_strip.md) · [7-Segment](seven_seg.md) · [UI (LVGL)](ui.md)

-   :material-gesture-tap-button:{ .lg .middle } **Input**

    ---

    [:octicons-arrow-right-24: Button](button.md) · [Encoder](encoder.md) · [USB HID](usb_hid.md)

-   :material-thermometer:{ .lg .middle } **Sensor**

    ---

    [:octicons-arrow-right-24: DHT](dht.md)

-   :material-volume-high:{ .lg .middle } **Actuator**

    ---

    [:octicons-arrow-right-24: Buzzer](buzzer.md)

-   :material-wifi:{ .lg .middle } **Connectivity**

    ---

    [:octicons-arrow-right-24: WiFi](wifi.md) · [WiFi Provisioning](wifi_prov.md) · [ESP-NOW](espnow.md) · [Bluetooth](bluetooth.md) · [BLE GATT](ble_gatt.md) · [mDNS](mdns.md)

-   :material-swap-vertical:{ .lg .middle } **Protocol**

    ---

    [:octicons-arrow-right-24: HTTP Client](http_client.md) · [HTTP Server](http_server.md) · [MQTT](mqtt.md) · [WebSocket Client](ws_client.md)

-   :material-cog:{ .lg .middle } **System**

    ---

    [:octicons-arrow-right-24: Console](console.md) · [Filesystem](fs.md) · [FAT Filesystem](fatfs.md) · [Power Management](power_mgmt.md) · [SNTP](sntp.md) · [OTA](ota.md) · [Local Control](local_ctrl.md)

</div>
