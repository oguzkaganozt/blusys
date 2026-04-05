# Hardware Smoke Tests

Use this guide for a compact manual validation pass on real hardware after the build matrix already passes.

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## What To Run

Minimum async validation set:

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
8. `examples/i2s_basic`
9. `examples/spi_loopback`
10. `examples/pwm_basic`
11. `examples/adc_basic`
12. `examples/timer_basic`
13. `examples/touch_basic`

## Basic Workflow

For each target and example:

1. build the example for the target
2. flash the board
3. monitor serial output
4. apply the required jumper or pin action
5. confirm the expected result

Helper-script path:

```sh
./build.sh examples/<name> <target>
./run.sh examples/<name> <serial-port> <target>
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

### `system_info`

- uptime, reset reason, and heap values print normally

### `gpio_basic`

- output changes state correctly
- input sampling matches the wiring used

### `gpio_interrupt`

Setup:

1. identify the configured interrupt input pin
2. toggle the input with the expected pull direction

Pass conditions:

- interrupt count increases
- final result shows `callback_swap_result: ok`
- no reboot, lockup, or watchdog reset occurs

### `uart_loopback`

Setup:

1. connect TX to RX

Pass conditions:

- transmitted bytes are read back correctly

### `uart_async`

Setup:

1. connect TX to RX

Pass conditions:

- TX callback reports success
- RX callback receives the expected payload
- final result shows `callback_swap_result: ok`
- close/shutdown completes cleanly

### `i2c_scan`

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

### `spi_loopback`

Setup:

1. connect MOSI to MISO

Pass conditions:

- transmitted bytes are received back correctly

### `pwm_basic`

- output is measurable and stable
- duty and frequency changes behave as expected

### `adc_basic`

- raw values change with input voltage
- millivolt readings are plausible when calibration is available

### `timer_basic`

- tick count increments at the expected interval
- final result shows `callback_swap_result: ok`
- no extra callbacks appear after stop

### `touch_basic`

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
