# V1 Core Modules

## Foundation

### `version`

Purpose:
- library version macros and version query helpers

### `error`

Purpose:
- public `blusys_err_t`
- error-to-string helper later if useful

### `target`

Purpose:
- target name
- target family information
- capability reporting for supported common features

### `system`

Purpose:
- restart
- basic sleep entry helpers when clearly common
- uptime and system info helpers if useful and stable

## GPIO

Scope:
- configure pin direction
- configure pull mode
- read and write pin levels
- register interrupts and callbacks in v1 async scope

Rules:
- pin-based, no handle model
- keep common setup functions short

## UART

Scope:
- open and close
- blocking read and write
- async TX and RX callbacks

Rules:
- handle-based
- common setup path should not require large config structs

## I2C Master

Scope:
- open and close bus
- blocking transmit, receive, and write-read transfers
- bus scan helper

Rules:
- master mode first
- hide ESP-IDF bus/device complexity behind a simpler surface

## SPI Master

Scope:
- open and close bus or device abstraction
- blocking full-duplex transfer for common use

Rules:
- clearly document same-handle concurrency rules
- defer advanced queueing and exotic modes unless required

## PWM

Scope:
- open or allocate PWM channel
- set frequency and duty
- start and stop if needed by the design

Rules:
- expose the normal PWM use case, not full LEDC complexity

## ADC

Scope:
- oneshot reads
- calibration-backed value conversion where practical

Rules:
- common subset only
- avoid ADC2 special-case exposure in the public API

## Timer

Scope:
- periodic and one-shot timers
- blocking control plus callback-based async behavior

Rules:
- hide GPTimer detail behind a simpler lifecycle

## Module Completion Checklist

Before a module is considered complete:
- public API documented
- example added
- guide added
- both targets build
- thread-safety contract documented
- limitations documented
