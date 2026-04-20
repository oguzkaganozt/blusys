# Console

Interactive UART REPL with command registration for live firmware inspection and control.

## Quick Example

```c
#include "blusys/blusys.h"

static int cmd_version(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("%s\n", blusys_version_string());
    return 0;
}

void app_main(void)
{
    blusys_console_t *console;
    blusys_console_config_t cfg = { 0 };  /* all defaults */
    blusys_console_open(&cfg, &console);

    blusys_console_cmd_t version_cmd = {
        .command = "version",
        .help    = "Print Blusys version",
        .handler = cmd_version,
    };
    blusys_console_register_cmd(console, &version_cmd);

    blusys_console_start(console);   /* never returns under normal use */
}
```

## Common Mistakes

- **registering a command after `start()`** — returns `BLUSYS_ERR_INVALID_STATE`; register all commands before starting
- **opening a second console** — the REPL backend is a singleton; the second `open()` fails
- **assuming the handler runs in the caller's task** — handlers run on the REPL task, one at a time
- **parsing `argv` as if it persists** — the argument strings are only valid for the duration of the handler call

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- `open()`, `register_cmd()`, and `start()` must be called from a single task, in order
- command handlers are invoked sequentially from the REPL task; they are never concurrent with each other
- it is safe to call Blusys APIs from within a command handler

## ISR Notes

No ISR-safe calls are defined for the console module.

## Limitations

- only one console instance is supported at a time (the ESP-IDF `esp_console` backend is a singleton)
- commands cannot be unregistered after `blusys_console_start()`
- there is no built-in argument parsing — handlers receive raw `argc`/`argv` and must parse flags themselves

## Example App

See `examples/validation/platform_lab/` (`plat_console` scenario).
