# Concurrency stress tests

Hardware-oriented stress tests live under `examples/validation/concurrency_suite/`. Pick the scenario in menuconfig (timer, I2C, SPI, UART).

## concurrency_suite (timer)

Two worker tasks race against the timer ISR: one repeatedly stops/starts the timer; the other swaps callbacks. Pass when `concurrent_result: ok` is printed and both workers report zero unexpected errors.

## concurrency_suite (SPI)

Two tasks call `blusys_spi_transfer()` on the same handle. MOSI must be wired to MISO (loopback). Pass when both tasks report zero errors and zero mismatches.

## concurrency_suite (I2C)

Two tasks call `blusys_i2c_master_probe()` on the same handle. No specific slave is required: `BLUSYS_OK`, `BLUSYS_ERR_IO`, and `BLUSYS_ERR_TIMEOUT` are valid per iteration.

## concurrency_suite (UART)

One task writes while another reads on the same UART with TX/RX loopback. Pass when bytes received match bytes sent and `concurrent_result: ok`.
