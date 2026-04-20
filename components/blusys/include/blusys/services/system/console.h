/**
 * @file console.h
 * @brief UART-backed interactive command console with line editing and history.
 *
 * Command handlers run on the console task; keep them short and use async
 * APIs for long-running work. Commands registered before ::blusys_console_start
 * are always available; later additions race with user input. See
 * docs/services/console.md.
 */

#ifndef BLUSYS_CONSOLE_H
#define BLUSYS_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "blusys/hal/error.h"

/** @brief Opaque handle to an open console. */
typedef struct blusys_console blusys_console_t;

/**
 * @brief Command handler signature.
 * @param argc  Argument count, including the command name at `argv[0]`.
 * @param argv  Argument vector; strings are owned by the console and valid
 *              only for the duration of the call.
 * @return `0` on success, non-zero on error (displayed to the user).
 */
typedef int (*blusys_console_cmd_handler_t)(int argc, char **argv);

/** @brief Command registration descriptor. */
typedef struct {
    const char                   *command; /**< Command name; no spaces allowed. */
    const char                   *help;    /**< One-line help text shown by `help`; may be NULL. */
    blusys_console_cmd_handler_t  handler; /**< Handler function. */
} blusys_console_cmd_t;

/** @brief Configuration for ::blusys_console_open. */
typedef struct {
    int         uart_num;         /**< UART peripheral index; `0` selects `UART_NUM_0`. */
    int         baud_rate;        /**< Baud rate; `0` selects 115200. */
    int         tx_pin;           /**< TX GPIO pin; `-1` selects the target default. */
    int         rx_pin;           /**< RX GPIO pin; `-1` selects the target default. */
    const char *prompt;           /**< Prompt string; NULL selects `"> "`. */
    int         max_line_length;  /**< Input line buffer; `0` selects 256. */
    int         history_size;     /**< Number of history entries; `0` selects 32. */
} blusys_console_config_t;

/**
 * @brief Open a console on the given UART and start the REPL task.
 *
 * The task is parked until ::blusys_console_start is called, so commands
 * can be registered first.
 *
 * @param config       Configuration.
 * @param out_console  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_INVALID_STATE` if a console is already open.
 */
blusys_err_t blusys_console_open(const blusys_console_config_t *config,
                                  blusys_console_t **out_console);

/**
 * @brief Stop the REPL, tear down the UART, and free the handle.
 * @param console  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p console is NULL.
 */
blusys_err_t blusys_console_close(blusys_console_t *console);

/**
 * @brief Register a command.
 *
 * The command name and help string must remain valid for the lifetime of
 * the console; the descriptor itself may be reused or go out of scope.
 *
 * @param console  Handle.
 * @param cmd      Command descriptor.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, or `BLUSYS_ERR_NO_MEM`.
 */
blusys_err_t blusys_console_register_cmd(blusys_console_t *console,
                                          const blusys_console_cmd_t *cmd);

/**
 * @brief Begin accepting user input on the console.
 * @param console  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`,
 *         `BLUSYS_ERR_INVALID_STATE` if already started.
 */
blusys_err_t blusys_console_start(blusys_console_t *console);

#ifdef __cplusplus
}
#endif

#endif
