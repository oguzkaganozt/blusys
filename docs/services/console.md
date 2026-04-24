# Console

UART REPL with command registration for live firmware inspection and control.

## At a glance

- one console instance at a time
- register commands before `start()`
- handlers run sequentially on the REPL task

## Quick example

```c
static int cmd_version(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("%s\n", blusys_version_string());
    return 0;
}

blusys_console_t *console;
blusys_console_config_t cfg = {0};
blusys_console_open(&cfg, &console);
blusys_console_register_cmd(console, &(blusys_console_cmd_t){
    .command = "version",
    .help = "Print Blusys version",
    .handler = cmd_version,
});
```

## Common mistakes

- registering a command after `start()`
- opening a second console
- treating `argv` as persistent

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- commands are invoked sequentially
- it is safe to call Blusys APIs from a handler

## Limitations

- no unregister after `start()`
- no built-in argument parsing

## Example app

`examples/validation/platform_lab/` (`plat_console`)
