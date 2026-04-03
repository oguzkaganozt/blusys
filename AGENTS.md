# AGENTS.md

This file is for coding agents working in this repository.
It summarizes the current build, validation, and coding rules for Blusys HAL.

## Project Summary

- Project name: `Blusys HAL`
- Language: C
- Build system: ESP-IDF v5.5 with CMake
- Current supported targets: `esp32c3`, `esp32s3`
- Main component: `components/blusys/`
- Current validation apps: `examples/smoke/`, `examples/system_info/`, `examples/gpio_basic/`, `examples/gpio_interrupt/`, `examples/uart_loopback/`, `examples/uart_async/`, `examples/i2c_scan/`, `examples/spi_loopback/`, `examples/pwm_basic/`, `examples/adc_basic/`, `examples/timer_basic/`

The repository is still early-stage.
Phase 1 foundation, Phase 2 foundational public modules, Phase 3 communication modules, and Phase 4 timing and analog modules are implemented.
Phase 5 async support is in progress and currently includes timer callbacks, GPIO interrupt callbacks, and UART async support.
Formal unit tests and lint scripts do not exist yet.

## Repository Layout

- `components/blusys/`: main ESP-IDF component
- `components/blusys/include/blusys/`: public headers
- `components/blusys/src/common/`: shared implementation
- `components/blusys/src/internal/`: internal-only headers and utilities
- `components/blusys/src/targets/esp32c3/`: C3-specific internals
- `components/blusys/src/targets/esp32s3/`: S3-specific internals
- `examples/smoke/`: build validation app
- `examples/system_info/`: `system` module example
- `examples/gpio_basic/`: `gpio` module example
- `examples/gpio_interrupt/`: `gpio` interrupt callback example
- `examples/uart_loopback/`: `uart` module example
- `examples/uart_async/`: `uart` async callback example
- `examples/i2c_scan/`: `i2c` master module example
- `examples/spi_loopback/`: `spi` master module example
- `examples/pwm_basic/`: `pwm` module example
- `examples/adc_basic/`: `adc` module example
- `examples/timer_basic/`: `timer` module example
- `docs/`: architecture, API rules, roadmap, workflow
- `PROGRESS.md`: implementation progress tracker
- `esp-idf-en-v5.5.4/`: bundled upstream ESP-IDF docs

## Source Of Truth

Before changing code, check these files:

- `README.md`
- `PROGRESS.md`
- `docs/architecture.md`
- `docs/api-design-rules.md`
- `docs/development-workflow.md`
- `docs/roadmap.md`

## Editor / Agent Rules

There are currently no repo-local Cursor rules:
- no `.cursorrules`
- no `.cursor/rules/`

There are currently no repo-local Copilot instructions:
- no `.github/copilot-instructions.md`

If such files are added later, treat them as part of the repository contract and update this file.

## Environment Setup

ESP-IDF must be exported before running builds.

Recommended local setup:

```sh
export IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.10_env
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
```

If the ESP-IDF Python environment fails during export, verify missing Python packages.
This repo previously required `tree_sitter` and `tree_sitter_c` in the local ESP-IDF Python env.
On this machine, `export.sh` may also look for a missing `idf5.5_py3.14_env`; setting `IDF_PYTHON_ENV_PATH` to the installed `idf5.5_py3.10_env` fixes it.

## Build Commands

### Build Smoke App For ESP32-C3

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/smoke -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 set-target esp32c3 build
```

### Build Smoke App For ESP32-S3

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 set-target esp32s3 build
```

### Build System Info Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/system_info -B build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig set-target esp32c3 build
idf.py -C examples/system_info -B build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig set-target esp32s3 build
```

### Build GPIO Basic Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/gpio_basic -B build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig set-target esp32c3 build
idf.py -C examples/gpio_basic -B build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig set-target esp32s3 build
```

### Build GPIO Interrupt Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/gpio_interrupt -B examples/gpio_interrupt/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig set-target esp32c3 build
idf.py -C examples/gpio_interrupt -B examples/gpio_interrupt/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig set-target esp32s3 build
```

### Build UART Loopback Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/uart_loopback -B examples/uart_loopback/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig set-target esp32c3 build
idf.py -C examples/uart_loopback -B examples/uart_loopback/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig set-target esp32s3 build
```

### Build UART Async Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/uart_async -B examples/uart_async/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig set-target esp32c3 build
idf.py -C examples/uart_async -B examples/uart_async/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig set-target esp32s3 build
```

### Build I2C Scan Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/i2c_scan -B build-esp32c3 set-target esp32c3 build
idf.py -C examples/i2c_scan -B build-esp32s3 set-target esp32s3 build
```

### Build SPI Loopback Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/spi_loopback -B build-esp32c3 set-target esp32c3 build
idf.py -C examples/spi_loopback -B build-esp32s3 set-target esp32s3 build
```

### Build PWM Basic Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/pwm_basic -B build-esp32c3 set-target esp32c3 build
idf.py -C examples/pwm_basic -B build-esp32s3 set-target esp32s3 build
```

### Build ADC Basic Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/adc_basic -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 set-target esp32c3 build
idf.py -C examples/adc_basic -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 set-target esp32s3 build
```

### Build Timer Basic Example

```sh
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/timer_basic -B examples/timer_basic/build-esp32c3 -DSDKCONFIG=build-esp32c3/sdkconfig set-target esp32c3 build
idf.py -C examples/timer_basic -B examples/timer_basic/build-esp32s3 -DSDKCONFIG=build-esp32s3/sdkconfig set-target esp32s3 build
```

### Rebuild After Clean

```sh
rm -rf build-esp32c3 build-esp32s3
rm -f examples/smoke/sdkconfig examples/smoke/sdkconfig.old
source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh
idf.py -C examples/smoke -B build-esp32c3 -DSDKCONFIG=sdkconfig.esp32c3 set-target esp32c3 build
idf.py -C examples/smoke -B build-esp32s3 -DSDKCONFIG=sdkconfig.esp32s3 set-target esp32s3 build
```

### Single-Target Validation

Use a single target build when changing code that should remain target-agnostic but you only need fast feedback.

Examples:

```sh
idf.py -C examples/smoke -B build-esp32c3 build
idf.py -C examples/smoke -B build-esp32s3 build
```

Use the matching build directory that was already configured with `set-target`.

## Test Commands

There is no formal unit test suite yet.
Current validation is build-based.

Current practical test matrix:
- build smoke app for `esp32c3`
- build smoke app for `esp32s3`
- build `system_info` for `esp32c3`
- build `system_info` for `esp32s3`
- build `gpio_basic` for `esp32c3`
- build `gpio_basic` for `esp32s3`
- build `gpio_interrupt` for `esp32c3`
- build `gpio_interrupt` for `esp32s3`
- build `uart_loopback` for `esp32c3`
- build `uart_loopback` for `esp32s3`
- build `uart_async` for `esp32c3`
- build `uart_async` for `esp32s3`
- build `i2c_scan` for `esp32c3`
- build `i2c_scan` for `esp32s3`
- build `spi_loopback` for `esp32c3`
- build `spi_loopback` for `esp32s3`
- build `pwm_basic` for `esp32c3`
- build `pwm_basic` for `esp32s3`
- build `adc_basic` for `esp32c3`
- build `adc_basic` for `esp32s3`
- build `timer_basic` for `esp32c3`
- build `timer_basic` for `esp32s3`

### Closest Equivalent To A Single Test

Because there is no unit test runner yet, the closest equivalent to a single test is a single smoke-app target build:

```sh
idf.py -C examples/smoke -B build-esp32c3 build
```

or:

```sh
idf.py -C examples/smoke -B build-esp32s3 build
```

When real tests are added, update this section with exact single-test commands.

## Lint / Formatting Commands

There is no repo-managed lint or formatting command yet.

Implications:
- do not invent a formatter config unless requested
- keep formatting consistent with nearby code
- keep includes, spacing, and brace style aligned with existing files

If `clang-format`, `clang-tidy`, or other tooling is added later, document the exact commands here.

## Public API Rules

- Public API is C only.
- All public symbols use the `blusys_` prefix.
- Public headers must not expose ESP-IDF-specific public types.
- Public API should be simpler than ESP-IDF, not a 1:1 wrapper.
- Prefer direct names like `blusys_gpio_write`.
- Use `blusys_err_t` for public error returns.
- Use raw GPIO numbers publicly.
- Use opaque handles for stateful peripherals.
- Keep public APIs stable and additive.

## Naming Conventions

- Public functions: `blusys_<module>_<verb>`
- Public enums and typedefs: `blusys_*_t`
- Public macros: `BLUSYS_*`
- Internal helpers: `blusys_*` if shared, otherwise use file-local `static`
- Files should be named after the module or purpose, for example `target.c`, `blusys_lock.h`

Examples:
- `blusys_gpio_write`
- `blusys_uart_open`
- `blusys_target_name`
- `BLUSYS_ERR_TIMEOUT`

## Formatting Style

- Use ASCII by default.
- Use 4 spaces for indentation.
- Put opening braces on the same line as the function or control statement.
- Prefer one declaration per line.
- Keep functions small when practical.
- Use blank lines to separate logical blocks.
- Avoid vertical alignment formatting that is hard to maintain.

Match the current style already present in `components/blusys/src/common/*.c` and `src/internal/*.c`.

## Include Rules

- Include the module's own header first when applicable.
- Then include related project internal headers.
- Then include ESP-IDF / FreeRTOS headers.
- Then include standard library headers if needed.

Important FreeRTOS rule:
- include `freertos/FreeRTOS.h` before `freertos/semphr.h` and similar FreeRTOS headers that require it

Keep include order stable and minimal.
Do not add unused includes.

## Type And Data Rules

- Prefer fixed-width integer types in public and internal code where width matters.
- Use `bool` for boolean state.
- Use `int` for raw GPIO-style public pin arguments when `-1` may be useful later.
- Keep public structs small and only add them when truly needed.
- Do not expose internal target capability structures in public headers.

## Error Handling Rules

- Return `blusys_err_t` from public functions unless there is a strong reason not to.
- Validate inputs early.
- Return `BLUSYS_ERR_INVALID_ARG` for bad pointers or invalid values.
- Return `BLUSYS_ERR_INVALID_STATE` when lifecycle or ownership is wrong.
- Translate backend errors internally.
- Do not leak `esp_err_t` in public headers.
- Keep the public error model small and documented.

## Concurrency And Lifecycle Rules

- Design thread safety from the beginning.
- Document thread-safety expectations per module.
- Use internal locks for handle-based modules when needed.
- Keep lifecycle explicit: `open`, `close`, `start`, `stop`, `read`, `write`, `transfer`.
- Protect close/deinit paths from concurrent active use.
- ISR-safe behavior must be explicit and documented.

## Implementation Guidance

- Start with the smallest correct implementation.
- Keep the public surface smaller than the backend surface.
- Localize target-specific behavior under `src/targets/` or internal layers.
- Do not introduce board-specific abstractions into the HAL.
- Do not add options just because ESP-IDF exposes them.
- Add examples and docs with each public module.

## Validation Expectations For Changes

At minimum, for code changes touching the component:
- build `examples/smoke` for `esp32c3`
- build `examples/smoke` for `esp32s3`

For code changes touching `system` or `gpio`:
- build `examples/system_info` for both targets
- build `examples/gpio_basic` for both targets
- build `examples/gpio_interrupt` for both targets

For code changes touching `uart`, `i2c`, or `spi`:
- build `examples/uart_loopback` for both targets
- build `examples/uart_async` for both targets
- build `examples/i2c_scan` for both targets
- build `examples/spi_loopback` for both targets

For code changes touching `pwm`:
- build `examples/pwm_basic` for both targets

For code changes touching `adc`:
- build `examples/adc_basic` for both targets

For code changes touching `timer`:
- build `examples/timer_basic` for both targets

For documentation-only changes:
- keep docs consistent with code and `PROGRESS.md`

For future module work:
- update docs
- update examples
- update `PROGRESS.md`
- keep `AGENTS.md` accurate if commands or rules change
