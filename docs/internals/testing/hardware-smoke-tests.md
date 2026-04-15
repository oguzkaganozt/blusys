# Hardware Smoke Tests

Use this guide for a compact manual validation pass on real hardware after the build matrix already passes.

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## What To Run

Minimum async validation set:

1. `examples/validation/hal_io_lab` (GPIO interrupt scenario)
2. `examples/reference/uart_basic` (async mode)
3. `examples/reference/hal` (timer scenario)

Recommended full smoke pass:

1. `examples/validation/smoke`
2. `examples/validation/hal_io_lab` (system_info scenario)
3. `examples/reference/hal` (GPIO scenario)
4. `examples/validation/hal_io_lab` (GPIO interrupt scenario)
5. `examples/reference/uart_basic` (loopback mode)
6. `examples/reference/uart_basic` (async mode)
7. `examples/reference/hal` (I2C scan scenario)
8. `examples/validation/i2s_basic`
9. `examples/reference/hal` (SPI loopback scenario)
10. `examples/reference/hal` (PWM scenario)
11. `examples/reference/hal` (ADC scenario)
12. `examples/validation/hal_io_lab` (DAC scenario)
13. `examples/reference/hal` (timer scenario)
14. `examples/validation/hal_io_lab` (touch scenario)

## Basic Workflow

For each target and example:

1. build the example for the target
2. flash the board
3. monitor serial output
4. apply the required jumper or pin action
5. confirm the expected result

Helper-script path:

```sh
blusys build examples/<name> <target>
blusys run   examples/<name> <serial-port> <target>
```

Direct `idf.py` pattern:

```sh
idf.py -C examples/<name> -B build-<target> set-target <target> build
idf.py -C examples/<name> -B build-<target> -p <serial-port> flash monitor
```

Exit the serial monitor with `Ctrl+]`.

## Board Preparation

Before testing:

1. confirm the target matches the connected chip
2. confirm the configured pins are safe for the actual board
3. avoid flash, PSRAM, boot-strap, USB, JTAG, and onboard peripheral pins
4. update example-local pin settings in `menuconfig` when needed

## Per-Example Checks

### `smoke`

- board boots normally
- reported target is correct
- capability lines look correct

### `hal_io_lab` (system_info)

- uptime, reset reason, and heap values print normally

### `hal` (GPIO)

- output changes state correctly
- input sampling matches the wiring used

### `hal_io_lab` (GPIO interrupt)

Setup:

1. identify the configured interrupt input pin
2. toggle the input with the expected pull direction

Pass conditions:

- interrupt count increases
- final result shows `callback_swap_result: ok`
- no reboot, lockup, or watchdog reset occurs

### `uart_basic` (loopback mode)

Setup:

1. connect TX to RX

Pass conditions:

- transmitted bytes are read back correctly

### `uart_basic` (async mode)

Setup:

1. connect TX to RX

Pass conditions:

- TX callback reports success
- RX callback receives the expected payload
- final result shows `callback_swap_result: ok`
- close/shutdown completes cleanly

### `hal` (I2C scan)

- expected devices appear at the expected addresses
- timeout behavior does not indicate bad wiring or missing pull-ups

### `i2s_basic`

Setup:

1. connect the configured BCLK, WS, and DOUT pins to an external I2S DAC or codec
2. connect MCLK as well if the external device requires it

Pass conditions:

- the example starts without open or start errors
- the external I2S device receives stable clocking
- the generated tone is audible or otherwise measurable at the external device output

### `hal` (SPI loopback)

Setup:

1. connect MOSI to MISO

Pass conditions:

- transmitted bytes are received back correctly

### `hal` (PWM)

- output is measurable and stable
- duty and frequency changes behave as expected

### `hal` (ADC)

- raw values change with input voltage
- millivolt readings are plausible when calibration is available

### `hal_io_lab` (DAC)

Setup:

1. use `esp32`
2. connect a multimeter or oscilloscope to GPIO25 or GPIO26

Pass conditions:

- the example starts without open errors on `esp32`
- the printed DAC value ramps up and down repeatedly
- the measured analog voltage changes with the ramp pattern

### `hal` (timer)

- tick count increments at the expected interval
- final result shows `callback_swap_result: ok`
- no extra callbacks appear after stop

### `hal_io_lab` (touch)

Setup:

1. select a touch-capable GPIO for the current target
2. connect or expose the matching touch pad or conductive area

Pass conditions:

- the example starts without open errors on `esp32` and `esp32s3`
- repeated readings are stable when untouched
- touching and releasing the pad produces a repeatable reading change

## What To Record

For each run, record:

- board name
- target
- serial port
- example
- wiring used
- result: `pass`, `fail`, or `blocked`
- short notes for anything unexpected

Use `hardware-validation-report-template.md` if you want a reusable format.

## Failure Triage

If a test fails:

1. confirm the target matches the board
2. confirm the configured pins are board-safe
3. confirm jumper wiring is correct
4. rebuild and reflash the example
5. compare behavior on another supported target if relevant
6. capture monitor output and note the exact example, board, and wiring
