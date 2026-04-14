#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"
#include "blusys/blusys.h"

#define CONSOLE_UART_NUM CONFIG_BLUSYS_CONSOLE_UART_NUM

static int cmd_hello(int argc, char **argv)
{
    printf("Hello from blusys console!\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    printf("target:    %s\n",  blusys_target_name());
    printf("cpu cores: %u\n",  blusys_target_cpu_cores());

    size_t free_heap = 0;
    blusys_system_free_heap_bytes(&free_heap);
    printf("free heap: %u bytes\n", (unsigned) free_heap);

    return 0;
}

void app_main(void)
{
    printf("Blusys console basic\ntarget: %s  uart: %d\n", blusys_target_name(), CONSOLE_UART_NUM);

    blusys_console_t *console = NULL;
    blusys_err_t err;

    const blusys_console_config_t config = {
        .uart_num = CONSOLE_UART_NUM,
        .prompt   = "esp> ",
    };

    err = blusys_console_open(&config, &console);
    if (err != BLUSYS_OK) {
        printf("blusys_console_open error: %s\n", blusys_err_string(err));
        return;
    }

    const blusys_console_cmd_t hello_cmd = {
        .command = "hello",
        .help    = "print a greeting message",
        .handler = cmd_hello,
    };
    const blusys_console_cmd_t info_cmd = {
        .command = "info",
        .help    = "print target info and free heap",
        .handler = cmd_info,
    };

    blusys_console_register_cmd(console, &hello_cmd);
    blusys_console_register_cmd(console, &info_cmd);

    err = blusys_console_start(console);
    if (err != BLUSYS_OK) {
        printf("blusys_console_start error: %s\n", blusys_err_string(err));
        blusys_console_close(console);
        return;
    }

    /* REPL runs in its own task — main task is no longer needed */
    vTaskDelete(NULL);
}
