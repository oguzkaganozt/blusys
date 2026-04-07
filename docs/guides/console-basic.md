# Add an Interactive Console to Your Firmware

## Problem Statement

You want to attach a live CLI to a running ESP32 firmware so that you can query state, trigger actions, and debug behaviour without reflashing — using only a UART connection.

## Prerequisites

- any supported target (ESP32, ESP32-C3, or ESP32-S3)
- a UART-to-USB adapter or built-in USB-UART bridge on the devkit (usually UART0)
- a terminal emulator that supports VT100 (e.g. `idf.py monitor`, `screen`, `minicom`)

## Minimal Example

```c
#include "blusys/blusys_all.h"

static int cmd_hello(int argc, char **argv)
{
    printf("Hello!\n");
    return 0;
}

void app_main(void)
{
    blusys_console_t *console;

    const blusys_console_config_t config = {
        .prompt = "esp> ",
    };

    blusys_console_open(&config, &console);

    const blusys_console_cmd_t hello = {
        .command = "hello",
        .help    = "print a greeting",
        .handler = cmd_hello,
    };
    blusys_console_register_cmd(console, &hello);

    blusys_console_start(console);

    /* REPL runs in its own task — delete main task */
    vTaskDelete(NULL);
}
```

## APIs Used

- `blusys_console_open()` — initialises the REPL and the built-in `help` command
- `blusys_console_register_cmd()` — registers a custom command before the REPL starts
- `blusys_console_start()` — launches the background task; must be called after all commands are registered

## Expected Runtime Behavior

- on boot the prompt (e.g. `esp> `) appears on the UART terminal
- typing `help` lists all registered commands with their descriptions
- typing a command name and pressing Enter calls the registered handler
- arrow keys scroll through command history; Tab may offer completions if hints are set
- the handler return value is printed as the command exit code when non-zero

## Common Mistakes

- calling `blusys_console_register_cmd()` after `blusys_console_start()` — this returns `BLUSYS_ERR_INVALID_STATE`; all commands must be registered first
- not deleting the main task — `vTaskDelete(NULL)` is the idiomatic way to hand control to the REPL task after startup
- using the same `uart_num` that another blusys driver is already using — pick a free UART port

## Example App

See `examples/console_basic/` for a runnable example with two commands (`hello` and `info`).

Build and run with the helper scripts or use the pattern in [Getting Started](getting-started.md).

## API Reference

For full type definitions and function signatures, see [Console API Reference](../modules/console.md).
