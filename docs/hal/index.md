# HAL + Drivers

Direct hardware access through the Blusys HAL and driver layers. These APIs are pure C with `blusys_err_t` return types.

For most product apps, the [App](../app/index.md) layer and [Capabilities](../app/capabilities.md) provide higher-level access. Use these APIs when you need direct hardware control.

## I/O & Communication

| Module | Description | Targets |
|--------|-------------|---------|
| [GPIO](gpio.md) | Digital input, output, pull resistors, ISR callbacks | All |
| [UART](uart.md) | Async serial communication | All |
| [I2C](i2c.md) | I2C master and slave | All |
| [SPI](spi.md) | SPI master and slave | All |
| [TWAI](twai.md) | CAN bus (TWAI) | All |
| [I2S](i2s.md) | Inter-IC Sound TX and RX | All |
| [RMT](rmt.md) | Remote control transceiver TX and RX | All |
| [1-Wire](one_wire.md) | Dallas 1-Wire bus | All |
| [GPIO Expander](gpio_expander.md) | I2C GPIO expander | All |
| [Touch](touch.md) | Capacitive touch pads | ESP32, S3 |

## Analog

| Module | Description | Targets |
|--------|-------------|---------|
| [ADC](adc.md) | Analog-to-digital conversion | All |
| [DAC](dac.md) | Digital-to-analog output | ESP32 |
| [SDM](sdm.md) | Sigma-delta modulation | All |
| [PWM](pwm.md) | Pulse-width modulation | All |

## Timers & Counters

| Module | Description | Targets |
|--------|-------------|---------|
| [Timer](timer.md) | General-purpose hardware timer | All |
| [PCNT](pcnt.md) | Pulse counter | ESP32, S3 |
| [MCPWM](mcpwm.md) | Motor control PWM | ESP32, S3 |

## Storage

| Module | Description | Targets |
|--------|-------------|---------|
| [NVS](nvs.md) | Non-volatile key-value storage | All |
| [SD/MMC](sdmmc.md) | SD card over MMC interface | ESP32, S3 |
| [SD over SPI](sd_spi.md) | SD card over SPI interface | All |

## Device

| Module | Description | Targets |
|--------|-------------|---------|
| [System](system.md) | Chip info, heap, reset reason | All |
| [Sleep](sleep.md) | Light and deep sleep modes | All |
| [Watchdog](wdt.md) | Task and interrupt watchdog | All |
| [Temp Sensor](temp_sensor.md) | Internal temperature sensor | C3, S3 |
| [eFuse](efuse.md) | One-time programmable fuses | All |
| [ULP](ulp.md) | Ultra-low-power coprocessor | ESP32, S3 |
| [USB Host](usb_host.md) | USB host controller | S3 |
| [USB Device](usb_device.md) | USB device (CDC, HID) | S3 |

## Drivers

| Module | Description | Targets |
|--------|-------------|---------|
| [LCD](lcd.md) | SPI/I2C display panels | All |
| [LED Strip](led_strip.md) | Addressable LED strips | All |
| [7-Segment](seven_seg.md) | 7-segment LED displays | All |
| [Button](button.md) | Debounced button driver | All |
| [Encoder](encoder.md) | Rotary encoder with button | All |
| [DHT](dht.md) | DHT temperature/humidity sensor | All |
| [Buzzer](buzzer.md) | PWM buzzer driver | All |
