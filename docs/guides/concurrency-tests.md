# Concurrency Tests

Four stress tests that verify concurrent access to the same peripheral handle from two FreeRTOS tasks. Run each with `blusys run examples/<name>`.

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

**Wire:** nothing — no external connections needed. The test only probes the bus and accepts "no device found" as a valid result.

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

**Pass:** `concurrent_result: ok`
