# Hardware Validation Report Template

Use this template to record manual hardware validation results for Blusys examples on:

- ESP32-C3
- ESP32-S3

It is intended for real-board smoke testing after the build matrix has already passed.

## Session Summary

- Date:
- Tester:
- Git commit:
- ESP-IDF version:
- Host machine:
- Notes:

## Boards Under Test

### Board 1

- Board name:
- Target:
- USB serial port:
- Power source:
- Cable used:
- Notes:

### Board 2

- Board name:
- Target:
- USB serial port:
- Power source:
- Cable used:
- Notes:

## Pin Configuration Used

Record the actual pin choices used during testing.

### ESP32-C3

- GPIO basic input pin:
- GPIO basic output pin:
- GPIO interrupt pin:
- UART loopback TX pin:
- UART loopback RX pin:
- UART async TX pin:
- UART async RX pin:
- PWM pin:
- ADC pin:
- Timer: no external pin required

### ESP32-S3

- GPIO basic input pin:
- GPIO basic output pin:
- GPIO interrupt pin:
- UART loopback TX pin:
- UART loopback RX pin:
- UART async TX pin:
- UART async RX pin:
- PWM pin:
- ADC pin:
- Timer: no external pin required

## Wiring Used

### ESP32-C3

- UART loopback jumper:
- UART async jumper:
- GPIO interrupt jumper/button setup:
- SPI loopback jumper:
- ADC voltage source:
- Other:

### ESP32-S3

- UART loopback jumper:
- UART async jumper:
- GPIO interrupt jumper/button setup:
- SPI loopback jumper:
- ADC voltage source:
- Other:

## Results Table

| Board | Target | Example | Result | Notes |
|---|---|---|---|---|
| Board name | esp32c3 | smoke | pass | |
| Board name | esp32c3 | gpio_interrupt | pass | |
| Board name | esp32c3 | uart_async | pass | |
| Board name | esp32c3 | timer_basic | pass | |
| Board name | esp32s3 | smoke | pass | |

Use `pass`, `fail`, or `blocked` in the `Result` column.

## Example Checklist

Record the observed behavior for each example.

### Smoke

- Board:
- Target:
- Booted successfully:
- Target name printed correctly:
- Capability list looked correct:
- Notes:

### System Info

- Board:
- Target:
- System information printed:
- Reset or stability issues:
- Notes:

### GPIO Basic

- Board:
- Target:
- Output toggled as expected:
- Input sampled as expected:
- Notes:

### GPIO Interrupt

- Board:
- Target:
- Interrupt fired on expected edge:
- Repeated triggers worked:
- Counts stopped when line was idle:
- Reset/flash rerun still worked:
- Notes:

### UART Loopback

- Board:
- Target:
- TX and RX jumper installed:
- Payload looped back correctly:
- Repeated runs worked:
- Notes:

### UART Async

- Board:
- Target:
- TX and RX jumper installed:
- TX callback completed:
- RX callback received expected payload:
- Final result was `async_result: ok`:
- Close/shutdown completed cleanly:
- Notes:

### I2C Scan

- Board:
- Target:
- Bus wiring:
- Devices found:
- Expected addresses seen:
- Notes:

### SPI Loopback

- Board:
- Target:
- MOSI-MISO jumper installed:
- Loopback payload matched:
- Notes:

### PWM Basic

- Board:
- Target:
- Output observed on scope or logic analyzer:
- Frequency looked correct:
- Duty looked correct:
- Notes:

### ADC Basic

- Board:
- Target:
- Raw values changed with input voltage:
- mV values looked plausible:
- Notes:

### Timer Basic

- Board:
- Target:
- Tick count incremented at expected rate:
- Timer stopped at expected count:
- No extra ticks after stop:
- Notes:

## Failures And Follow-Up

For every failure or blocked test, record:

- example:
- board:
- target:
- exact wiring:
- exact console output:
- reproduction steps:
- suspected cause:
- follow-up action:

## Sign-Off

- ESP32-C3 smoke set complete:
- ESP32-S3 smoke set complete:
- Phase 5 async examples validated on hardware:
- Remaining blockers:
- Recommendation:
  - keep Phase 5 in progress
  - or mark Phase 5 complete
