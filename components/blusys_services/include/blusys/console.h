#ifndef BLUSYS_CONSOLE_H
#define BLUSYS_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "blusys/error.h"

typedef struct blusys_console blusys_console_t;

typedef int (*blusys_console_cmd_handler_t)(int argc, char **argv);

typedef struct {
    const char                   *command; /* no spaces allowed             */
    const char                   *help;    /* shown by 'help'; may be NULL  */
    blusys_console_cmd_handler_t  handler;
} blusys_console_cmd_t;

typedef struct {
    int         uart_num;        /* 0 → UART_NUM_0          */
    int         baud_rate;       /* 0 → 115200              */
    int         tx_pin;          /* -1 → target default     */
    int         rx_pin;          /* -1 → target default     */
    const char *prompt;          /* NULL → "> "             */
    int         max_line_length; /* 0 → 256                 */
    int         history_size;    /* 0 → 32                  */
} blusys_console_config_t;

blusys_err_t blusys_console_open(const blusys_console_config_t *config,
                                  blusys_console_t **out_console);
blusys_err_t blusys_console_close(blusys_console_t *console);
blusys_err_t blusys_console_register_cmd(blusys_console_t *console,
                                          const blusys_console_cmd_t *cmd);
blusys_err_t blusys_console_start(blusys_console_t *console);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_CONSOLE_H */
