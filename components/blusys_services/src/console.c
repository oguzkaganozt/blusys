#include "blusys/console.h"

#include <stdlib.h>

#include "blusys/internal/blusys_esp_err.h"

#include "esp_console.h"

#define CONSOLE_DEFAULT_BAUD_RATE    115200
#define CONSOLE_DEFAULT_MAX_LINE_LEN 256
#define CONSOLE_DEFAULT_HISTORY_SIZE 32
#define CONSOLE_DEFAULT_PROMPT       "> "

struct blusys_console {
    esp_console_repl_t *repl;
    bool                started;
};

blusys_err_t blusys_console_open(const blusys_console_config_t *config,
                                  blusys_console_t **out_console)
{
    if (!config || !out_console) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_console_t *h = calloc(1, sizeof(*h));
    if (!h) {
        return BLUSYS_ERR_NO_MEM;
    }

    int baud_rate       = (config->baud_rate      > 0) ? config->baud_rate      : CONSOLE_DEFAULT_BAUD_RATE;
    int max_line_length = (config->max_line_length > 0) ? config->max_line_length : CONSOLE_DEFAULT_MAX_LINE_LEN;
    int history_size    = (config->history_size    > 0) ? config->history_size    : CONSOLE_DEFAULT_HISTORY_SIZE;
    const char *prompt  = config->prompt ? config->prompt : CONSOLE_DEFAULT_PROMPT;

    esp_console_dev_uart_config_t uart_cfg = {
        .channel     = config->uart_num,
        .baud_rate   = baud_rate,
        .tx_gpio_num = config->tx_pin,
        .rx_gpio_num = config->rx_pin,
    };

    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt             = prompt;
    repl_cfg.max_history_len    = (uint32_t) history_size;
    repl_cfg.max_cmdline_length = (size_t) max_line_length;

    esp_err_t e = esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &h->repl);
    if (e != ESP_OK) {
        free(h);
        return blusys_translate_esp_err(e);
    }

    e = esp_console_register_help_command();
    if (e != ESP_OK) {
        h->repl->del(h->repl);
        free(h);
        return blusys_translate_esp_err(e);
    }

    *out_console = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_console_close(blusys_console_t *console)
{
    if (!console) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    /* repl->del() tears down the REPL task and calls esp_console_deinit() internally */
    esp_err_t e = console->repl->del(console->repl);
    free(console);
    return blusys_translate_esp_err(e);
}

blusys_err_t blusys_console_register_cmd(blusys_console_t *console,
                                          const blusys_console_cmd_t *cmd)
{
    if (!console || !cmd || !cmd->command || !cmd->handler) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (console->started) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    const esp_console_cmd_t esp_cmd = {
        .command = cmd->command,
        .help    = cmd->help,
        .func    = cmd->handler,
    };

    return blusys_translate_esp_err(esp_console_cmd_register(&esp_cmd));
}

blusys_err_t blusys_console_start(blusys_console_t *console)
{
    if (!console) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (console->started) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_err_t e = esp_console_start_repl(console->repl);
    if (e == ESP_OK) {
        console->started = true;
    }
    return blusys_translate_esp_err(e);
}
