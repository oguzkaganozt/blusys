# Hardware Smoke Tests

This guide describes how to run the Blusys hardware smoke tests on both supported targets:

- ESP32-C3
- ESP32-S3

It is written for quick manual validation on real boards after the build matrix has already passed.

## Goals

Use this guide to verify:

- examples boot correctly on real hardware
- public APIs behave as documented
- callback and lifecycle behavior is sane on actual boards
- the same smoke flow works on both C3 and S3

## Prerequisites

You need:

- one ESP32-C3 board
- one ESP32-S3 board
- one USB cable per board
- jumper wires
- a host machine with ESP-IDF exported
- this repository checked out locally

Recommended shell setup:

```sh
ls ~/.espressif/python_env/
export IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/<your-idf-env>
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
```

For example, on this machine the installed env is `idf5.5_py3.12_env`.

List serial ports before flashing:

```sh
ls /dev/ttyACM* /dev/ttyUSB*
```

Examples below use these placeholders:

- `PORT_C3` for the ESP32-C3 serial device
- `PORT_S3` for the ESP32-S3 serial device

Typical examples:

- `PORT_C3=/dev/ttyACM0`
- `PORT_S3=/dev/ttyACM1`

## General Test Flow

Run the same sequence on both boards:

1. build the example for the target
2. flash the board
3. open serial monitor
4. perform the required wiring or pin action
5. confirm expected console output

General flash pattern:

```sh
idf.py -C <example> -B <build-dir> -p <serial-port> flash monitor
```

Exit the serial monitor with `Ctrl+]`.

## Board-Specific Pin Selection

The example apps use Kconfig defaults for safe starter pins, but you should still confirm they make sense for your actual board.

Before hardware testing:

1. check the board schematic or pinout
2. avoid flash, PSRAM, boot-strap, USB, and onboard peripheral pins
3. if needed, change example-local Kconfig values with `idf.py menuconfig`

Important:

- the defaults are repo defaults, not guaranteed board-safe for every devkit variant
- loopback examples require a physical jumper between the configured pins

## Minimum Phase 5 Smoke Matrix

Run these on both ESP32-C3 and ESP32-S3:

1. `examples/gpio_interrupt`
2. `examples/uart_async`
3. `examples/timer_basic`

Recommended full smoke pass:

1. `examples/smoke`
2. `examples/system_info`
3. `examples/gpio_basic`
4. `examples/gpio_interrupt`
5. `examples/uart_loopback`
6. `examples/uart_async`
7. `examples/i2c_scan`
8. `examples/spi_loopback`
9. `examples/pwm_basic`
10. `examples/adc_basic`
11. `examples/timer_basic`

## Smoke Example

Use this first to confirm the board, serial path, and toolchain are all fine.

ESP32-C3:

```sh
idf.py -C examples/smoke -B examples/smoke/build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 -p PORT_C3 flash monitor
```

ESP32-S3:

```sh
idf.py -C examples/smoke -B examples/smoke/build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 -p PORT_S3 flash monitor
```

Expected result:

- board boots normally
- target name prints correctly
- capability reporting matches the implemented modules

## GPIO Interrupt Test

Purpose:

- validate ISR callback registration
- validate task wakeup from ISR callback path
- validate repeated interrupt delivery

Default behavior:

- example configures one input pin
- example installs an any-edge interrupt callback
- callback notifies a task
- task prints incrementing interrupt counts

ESP32-C3:

```sh
idf.py -C examples/gpio_interrupt -B examples/gpio_interrupt/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig -p PORT_C3 flash monitor
```

ESP32-S3:

```sh
idf.py -C examples/gpio_interrupt -B examples/gpio_interrupt/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig -p PORT_S3 flash monitor
```

Setup:

1. identify the configured interrupt input pin from the example log or Kconfig
2. if pull-up is enabled, momentarily jumper the input pin to `GND`
3. then release it back high
4. repeat several times

Expected result:

- `interrupt_count=` increases on each edge event
- no reboot, watchdog reset, or lockup occurs
- repeated toggling continues to work

Good checks:

1. toggle the line multiple times in quick succession
2. leave the line idle and confirm counts stop increasing
3. reflash and repeat on the other target

## UART Loopback Test

Purpose:

- validate blocking UART TX/RX path

ESP32-C3:

```sh
idf.py -C examples/uart_loopback -B examples/uart_loopback/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig -p PORT_C3 flash monitor
```

ESP32-S3:

```sh
idf.py -C examples/uart_loopback -B examples/uart_loopback/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig -p PORT_S3 flash monitor
```

Setup:

1. identify the configured TX and RX pins
2. install a jumper from TX to RX
3. reset the board if needed after wiring

Expected result:

- transmitted bytes are read back successfully
- the reported payload matches what was sent

## UART Async Test

Purpose:

- validate async TX completion callback
- validate task-context RX callback delivery
- validate loopback payload correctness
- validate close after async activity

ESP32-C3:

```sh
idf.py -C examples/uart_async -B examples/uart_async/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig -p PORT_C3 flash monitor
```

ESP32-S3:

```sh
idf.py -C examples/uart_async -B examples/uart_async/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig -p PORT_S3 flash monitor
```

Setup:

1. identify the configured TX and RX pins
2. install a jumper from TX to RX
3. boot the example

Expected result:

- TX callback completes and reports `BLUSYS_OK`
- RX callback receives the expected byte string
- final result prints `async_result: ok`
- board does not hang during shutdown

Good checks:

1. run the example multiple times in a row
2. power-cycle and rerun
3. repeat on both targets with the same logic

## Timer Callback Test

Purpose:

- validate periodic timer callback path
- validate ISR-to-task notification flow
- validate stop and close behavior after repeated firing

ESP32-C3:

```sh
idf.py -C examples/timer_basic -B examples/timer_basic/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig -p PORT_C3 flash monitor
```

ESP32-S3:

```sh
idf.py -C examples/timer_basic -B examples/timer_basic/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig -p PORT_S3 flash monitor
```

Expected result:

- `tick=` increments at the expected interval
- the example stops after the configured tick limit
- no extra callbacks appear after stop
- board remains stable after timer close

## Optional Module Smoke Tests

Use these when you want broader coverage beyond the async Phase 5 set.

### System Info

Expected result:

- target and system information print correctly

### GPIO Basic

Expected result:

- output toggling and input sampling behave as expected for the chosen board wiring

### I2C Scan

Expected result:

- attached devices appear at the expected bus addresses

### SPI Loopback

Setup:

1. jumper MOSI to MISO
2. keep the example's configured SPI pins board-safe

Expected result:

- transmitted bytes are received back correctly

### PWM Basic

Expected result:

- PWM output can be measured on a scope or logic analyzer
- frequency and duty appear stable

### ADC Basic

Expected result:

- raw reads change when the input voltage changes
- mV reads are plausible for the configured attenuation and source

## Runtime Checks To Record

For each board, record:

- board name
- target (`esp32c3` or `esp32s3`)
- serial port used
- example tested
- wiring used
- pass or fail
- short notes for any unexpected behavior

Suggested table:

| Board | Target | Example | Wiring | Result | Notes |
|---|---|---|---|---|---|
| Dev board name | esp32c3 | uart_async | TX-RX jumper | pass | payload matched |

For a reusable report format, see `hardware-validation-report-template.md`.

## Failure Triage

If a hardware smoke test fails:

1. confirm the configured pins are valid for that board
2. confirm the jumper wiring is correct
3. confirm the serial port is correct
4. rebuild and reflash the example
5. compare behavior on the other target
6. capture the monitor output and note the exact example and board

Common causes:

- wrong board pin choice
- missing TX/RX or loopback jumper
- boot-strap or reserved pin conflict
- stale build directory after target changes

## Phase 5 Exit Recommendation

After this guide is run successfully on both C3 and S3 for:

- `gpio_interrupt`
- `uart_async`
- `timer_basic`

you can reasonably move Phase 5 from build-complete to hardware-validated, then do the final review/fix pass before marking the phase complete.
