#if CONFIG_PLATFORM_LAB_SCENARIO_STORAGE
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "blusys/blusys.h"

#if CONFIG_BLUSYS_STORAGE_BASIC_BACKEND_SPIFFS

#define MOUNT_POINT "/fs"
#define FILE_NAME   "hello.txt"

static void run_spiffs(void)
{
    printf("blusys storage_basic (SPIFFS) on %s\n", blusys_target_name());

    blusys_fs_t *fs = NULL;
    blusys_fs_config_t config = {
        .base_path              = MOUNT_POINT,
        .partition_label        = NULL,
        .max_files              = 5,
        .format_if_mount_failed = true,
    };

    blusys_err_t err = blusys_fs_mount(&config, &fs);
    if (err != BLUSYS_OK) {
        printf("mount failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("mounted at %s\n", MOUNT_POINT);

    const char *msg = "Hello, Blusys FS!\n";
    err = blusys_fs_write(fs, FILE_NAME, msg, strlen(msg));
    printf("write: %s\n", blusys_err_string(err));

    bool exists = false;
    blusys_fs_exists(fs, FILE_NAME, &exists);
    printf("exists: %s\n", exists ? "yes" : "no");

    size_t sz = 0;
    blusys_fs_size(fs, FILE_NAME, &sz);
    printf("size: %u bytes\n", (unsigned) sz);

    char buf[64] = {0};
    size_t n = 0;
    err = blusys_fs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("read (%u bytes): %s", (unsigned) n, buf);

    const char *extra = "Appended line.\n";
    blusys_fs_append(fs, FILE_NAME, extra, strlen(extra));

    memset(buf, 0, sizeof(buf));
    blusys_fs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("after append (%u bytes):\n%s", (unsigned) n, buf);

    err = blusys_fs_remove(fs, FILE_NAME);
    printf("remove: %s\n", blusys_err_string(err));

    blusys_fs_exists(fs, FILE_NAME, &exists);
    printf("exists after remove: %s\n", exists ? "yes" : "no");

    blusys_fs_unmount(fs);
    printf("unmounted\n");
}

#elif CONFIG_BLUSYS_STORAGE_BASIC_BACKEND_FATFS

#define MOUNT_POINT  "/ffat"
#define FILE_NAME    "hello.txt"
#define SUBDIR_NAME  "logs"
#define SUBDIR_FILE  "logs/boot.txt"

static void list_entry(const char *name, void *user_data)
{
    (void) user_data;
    printf("  entry: %s\n", name);
}

static void run_fatfs(void)
{
    printf("blusys storage_basic (FAT) on %s\n", blusys_target_name());

    blusys_fatfs_t *fs = NULL;
    blusys_fatfs_config_t config = {
        .base_path              = MOUNT_POINT,
        .partition_label        = NULL,
        .max_files              = 5,
        .format_if_mount_failed = true,
        .allocation_unit_size   = 0,
    };

    blusys_err_t err = blusys_fatfs_mount(&config, &fs);
    if (err != BLUSYS_OK) {
        printf("mount failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("mounted at %s\n", MOUNT_POINT);

    const char *msg = "Hello, Blusys FATFS!\n";
    err = blusys_fatfs_write(fs, FILE_NAME, msg, strlen(msg));
    printf("write: %s\n", blusys_err_string(err));

    bool exists = false;
    blusys_fatfs_exists(fs, FILE_NAME, &exists);
    printf("exists: %s\n", exists ? "yes" : "no");

    size_t sz = 0;
    blusys_fatfs_size(fs, FILE_NAME, &sz);
    printf("size: %u bytes\n", (unsigned) sz);

    char buf[64] = {0};
    size_t n = 0;
    blusys_fatfs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("read (%u bytes): %s", (unsigned) n, buf);

    const char *extra = "Appended line.\n";
    blusys_fatfs_append(fs, FILE_NAME, extra, strlen(extra));
    memset(buf, 0, sizeof(buf));
    blusys_fatfs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("after append (%u bytes):\n%s", (unsigned) n, buf);

    err = blusys_fatfs_remove(fs, FILE_NAME);
    printf("remove: %s\n", blusys_err_string(err));
    blusys_fatfs_exists(fs, FILE_NAME, &exists);
    printf("exists after remove: %s\n", exists ? "yes" : "no");

    err = blusys_fatfs_mkdir(fs, SUBDIR_NAME);
    printf("mkdir %s: %s\n", SUBDIR_NAME, blusys_err_string(err));

    const char *log = "boot event\n";
    blusys_fatfs_write(fs, SUBDIR_FILE, log, strlen(log));

    printf("root entries:\n");
    blusys_fatfs_listdir(fs, NULL, list_entry, NULL);

    printf("%s/ entries:\n", SUBDIR_NAME);
    blusys_fatfs_listdir(fs, SUBDIR_NAME, list_entry, NULL);

    blusys_fatfs_remove(fs, SUBDIR_FILE);

    blusys_fatfs_unmount(fs);
    printf("unmounted\n");
}

#endif

void run_platform_storage(void)
{
#if CONFIG_BLUSYS_STORAGE_BASIC_BACKEND_SPIFFS
    run_spiffs();
#elif CONFIG_BLUSYS_STORAGE_BASIC_BACKEND_FATFS
    run_fatfs();
#endif
}

#else
void run_platform_storage(void) {}
#endif /* CONFIG_PLATFORM_LAB_SCENARIO_STORAGE */
