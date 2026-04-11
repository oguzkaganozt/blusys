#include <stdio.h>
#include <string.h>

#include "blusys/blusys.h"

#define MOUNT_POINT  "/sdcard"
#define FILE_NAME    "hello.txt"
#define SUBDIR_NAME  "logs"
#define SUBDIR_FILE  "logs/boot.txt"

static void list_entry(const char *name, void *user_data)
{
    (void)user_data;
    printf("  entry: %s\n", name);
}

void app_main(void)
{
    printf("blusys sd_spi_basic on %s\n", blusys_target_name());

    blusys_sd_spi_t *sd = NULL;
    blusys_sd_spi_config_t config = {
        .base_path              = MOUNT_POINT,
        .spi_host               = CONFIG_SD_SPI_HOST,
        .mosi_pin               = CONFIG_SD_SPI_MOSI_PIN,
        .miso_pin               = CONFIG_SD_SPI_MISO_PIN,
        .sclk_pin               = CONFIG_SD_SPI_SCLK_PIN,
        .cs_pin                 = CONFIG_SD_SPI_CS_PIN,
        .freq_hz                = 0,      /* use default (20 MHz) */
        .max_files              = 5,
        .format_if_mount_failed = false,
        .allocation_unit_size   = 0,      /* use default (16384) */
    };

    blusys_err_t err = blusys_sd_spi_mount(&config, &sd);
    if (err != BLUSYS_OK) {
        printf("mount failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("mounted at %s\n", MOUNT_POINT);

    /* Write a file at the root */
    const char *msg = "Hello, Blusys SD/SPI!\n";
    err = blusys_sd_spi_write(sd, FILE_NAME, msg, strlen(msg));
    printf("write: %s\n", blusys_err_string(err));

    /* Exists and size */
    bool exists = false;
    blusys_sd_spi_exists(sd, FILE_NAME, &exists);
    printf("exists: %s\n", exists ? "yes" : "no");

    size_t sz = 0;
    blusys_sd_spi_size(sd, FILE_NAME, &sz);
    printf("size: %u bytes\n", (unsigned)sz);

    /* Read back */
    char buf[64] = {0};
    size_t n = 0;
    blusys_sd_spi_read(sd, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("read (%u bytes): %s", (unsigned)n, buf);

    /* Append */
    const char *extra = "Appended line.\n";
    blusys_sd_spi_append(sd, FILE_NAME, extra, strlen(extra));
    memset(buf, 0, sizeof(buf));
    blusys_sd_spi_read(sd, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("after append (%u bytes):\n%s", (unsigned)n, buf);

    /* Remove root file */
    err = blusys_sd_spi_remove(sd, FILE_NAME);
    printf("remove: %s\n", blusys_err_string(err));
    blusys_sd_spi_exists(sd, FILE_NAME, &exists);
    printf("exists after remove: %s\n", exists ? "yes" : "no");

    /* Subdirectory and nested file */
    err = blusys_sd_spi_mkdir(sd, SUBDIR_NAME);
    printf("mkdir %s: %s\n", SUBDIR_NAME, blusys_err_string(err));

    const char *log = "boot event\n";
    blusys_sd_spi_write(sd, SUBDIR_FILE, log, strlen(log));

    /* listdir root */
    printf("root entries:\n");
    blusys_sd_spi_listdir(sd, NULL, list_entry, NULL);

    /* listdir subdir */
    printf("%s/ entries:\n", SUBDIR_NAME);
    blusys_sd_spi_listdir(sd, SUBDIR_NAME, list_entry, NULL);

    /* Cleanup */
    blusys_sd_spi_remove(sd, SUBDIR_FILE);

    blusys_sd_spi_unmount(sd);
    printf("unmounted\n");
}
