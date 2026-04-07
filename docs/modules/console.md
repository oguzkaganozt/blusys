# Console

Interactive UART REPL with command registration for live firmware inspection and control.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Add an Interactive Console](../guides/console-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_console_config_t`

```c
typedef struct {
    int         uart_num;        /* UART port number;             0  → UART_NUM_0  */
    int         baud_rate;       /* baud rate;                    0  → 115200       */
    int         tx_pin;          /* TX GPIO; -1 → use target default                */
    int         rx_pin;          /* RX GPIO; -1 → use target default                */
    const char *prompt;          /* prompt string;             NULL  → "> "         */
    int         max_line_length; /* maximum input line length;    0  → 256          */
    int         history_size;    /* command history depth;        0  → 32           */
} blusys_console_config_t;
```

### `blusys_console_cmd_t`

```c
typedef struct {
    const char                   *command; /* command name; no spaces allowed      */
    const char                   *help;    /* one-line description shown by 'help' */
    blusys_console_cmd_handler_t  handler; /* called when the command is typed     */
} blusys_console_cmd_t;
```

### `blusys_console_cmd_handler_t`

```c
typedef int (*blusys_console_cmd_handler_t)(int argc, char **argv);
```

Standard `argc`/`argv` handler. Return `0` on success; non-zero values are printed as the command exit code.

## Functions

### `blusys_console_open`

```c
blusys_err_t blusys_console_open(const blusys_console_config_t *config,
                                  blusys_console_t **out_console);
```

Initialises the UART REPL and registers the built-in `help` command. Does **not** start the background task — call `blusys_console_start()` after registering all commands.

**Parameters:**
- `config` — console configuration (required)
- `out_console` — receives the console handle on success

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_INTERNAL` on driver failure.

---

### `blusys_console_close`

```c
blusys_err_t blusys_console_close(blusys_console_t *console);
```

Stops the REPL task and frees all resources. The handle is invalid after this call.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `console` is NULL.

---

### `blusys_console_register_cmd`

```c
blusys_err_t blusys_console_register_cmd(blusys_console_t *console,
                                          const blusys_console_cmd_t *cmd);
```

Registers a command. Must be called after `blusys_console_open()` and before `blusys_console_start()`.

**Parameters:**
- `console` — handle returned by `blusys_console_open()`
- `cmd` — command descriptor; `command` and `handler` must be non-NULL; `help` may be NULL

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any required field is NULL, `BLUSYS_ERR_INVALID_STATE` if `blusys_console_start()` has already been called.

---

### `blusys_console_start`

```c
blusys_err_t blusys_console_start(blusys_console_t *console);
```

Launches the REPL background task. The task reads lines from UART, matches them against registered commands, and calls the appropriate handler. After this call the caller's task is free to continue or delete itself.

**Returns:** `BLUSYS_ERR_INVALID_STATE` if already started.

## Lifecycle

```
blusys_console_open()
    ↓
blusys_console_register_cmd() × N
    ↓
blusys_console_start()          ← REPL task begins
    ↓
(REPL runs indefinitely)
    ↓
blusys_console_close()          ← stops task, frees handle
```

## Thread Safety

- `blusys_console_open()`, `blusys_console_register_cmd()`, and `blusys_console_start()` must be called from a single task in the order shown above.
- Command handlers are invoked sequentially from the REPL task; they are never concurrent with each other.
- It is safe to call blusys APIs from within a command handler.

## Limitations

- Only one console instance is supported at a time (ESP-IDF `esp_console` is a singleton internally).
- Commands cannot be unregistered after `blusys_console_start()`.
- There is no built-in argument parsing — handlers receive raw `argc`/`argv` and must parse flags themselves.

## Example App

See `examples/console_basic/`.
