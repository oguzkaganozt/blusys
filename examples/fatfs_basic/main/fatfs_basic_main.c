#include <stdio.h>
#include <string.h>

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"

#define MOUNT_POINT  "/ffat"
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
    printf("blusys fatfs_basic on %s\n", blusys_target_name());

    /* Mount FAT on internal flash */
    blusys_fatfs_t *fs = NULL;
    blusys_fatfs_config_t config = {
        .base_path              = MOUNT_POINT,
        .partition_label        = NULL,   /* uses default "ffat" */
        .max_files              = 5,
        .format_if_mount_failed = true,
        .allocation_unit_size   = 0,      /* use default (4096) */
    };

    blusys_err_t err = blusys_fatfs_mount(&config, &fs);
    if (err != BLUSYS_OK) {
        printf("mount failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("mounted at %s\n", MOUNT_POINT);

    /* Write a file at the root */
    const char *msg = "Hello, Blusys FATFS!\n";
    err = blusys_fatfs_write(fs, FILE_NAME, msg, strlen(msg));
    printf("write: %s\n", blusys_err_string(err));

    /* Exists and size */
    bool exists = false;
    blusys_fatfs_exists(fs, FILE_NAME, &exists);
    printf("exists: %s\n", exists ? "yes" : "no");

    size_t sz = 0;
    blusys_fatfs_size(fs, FILE_NAME, &sz);
    printf("size: %u bytes\n", (unsigned)sz);

    /* Read back */
    char buf[64] = {0};
    size_t n = 0;
    blusys_fatfs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("read (%u bytes): %s", (unsigned)n, buf);

    /* Append */
    const char *extra = "Appended line.\n";
    blusys_fatfs_append(fs, FILE_NAME, extra, strlen(extra));
    memset(buf, 0, sizeof(buf));
    blusys_fatfs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("after append (%u bytes):\n%s", (unsigned)n, buf);

    /* Remove root file */
    err = blusys_fatfs_remove(fs, FILE_NAME);
    printf("remove: %s\n", blusys_err_string(err));
    blusys_fatfs_exists(fs, FILE_NAME, &exists);
    printf("exists after remove: %s\n", exists ? "yes" : "no");

    /* Subdirectory and nested file */
    err = blusys_fatfs_mkdir(fs, SUBDIR_NAME);
    printf("mkdir %s: %s\n", SUBDIR_NAME, blusys_err_string(err));

    const char *log = "boot event\n";
    blusys_fatfs_write(fs, SUBDIR_FILE, log, strlen(log));

    /* listdir root */
    printf("root entries:\n");
    blusys_fatfs_listdir(fs, NULL, list_entry, NULL);

    /* listdir subdir */
    printf("%s/ entries:\n", SUBDIR_NAME);
    blusys_fatfs_listdir(fs, SUBDIR_NAME, list_entry, NULL);

    /* Cleanup */
    blusys_fatfs_remove(fs, SUBDIR_FILE);

    blusys_fatfs_unmount(fs);
    printf("unmounted\n");
}
