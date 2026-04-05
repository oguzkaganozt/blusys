# Concurrency Tests

Four stress tests that verify concurrent access to the same peripheral handle from two FreeRTOS tasks. Run each with `./run.sh examples/<name>`.

---

## concurrency_spi

**Wire:** MOSI → MISO (one jumper)

| Target   | MOSI | MISO |
|----------|------|------|
| ESP32    | 23   | 19   |
| ESP32-C3 | 6    | 5    |
| ESP32-S3 | 1    | 2    |

**Pass:** `concurrent_result: ok`

---

## concurrency_i2c

**Wire:** SDA and SCL each need a pull-up to 3.3 V (4.7 kΩ). No device required — absent device is a valid result.

| Target   | SDA | SCL |
|----------|-----|-----|
| ESP32    | 21  | 22  |
| ESP32-C3 | 8   | 9   |
| ESP32-S3 | 8   | 9   |

**Pass:** `concurrent_result: ok`

---

## concurrency_timer

**Wire:** nothing — no external connections needed.

**Pass:** `concurrent_result: ok`

---

## concurrency_uart

**Wire:** TX → RX (one jumper)

| Target   | TX | RX |
|----------|----|----|
| ESP32    | 17 | 16 |
| ESP32-C3 | 21 | 20 |
| ESP32-S3 | 1  | 2  |

**Pass:** `concurrent_result: ok` and `bytes_sent == bytes_received`
