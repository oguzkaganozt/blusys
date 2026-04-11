#include <stdio.h>
#include <string.h>

#include "blusys/blusys.h"
#include "blusys/blusys_services.h"

#define MOUNT_POINT "/fs"
#define FILE_NAME   "hello.txt"

void app_main(void)
{
    printf("blusys fs_basic on %s\n", blusys_target_name());

    /* Mount SPIFFS */
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

    /* Write */
    const char *msg = "Hello, Blusys FS!\n";
    err = blusys_fs_write(fs, FILE_NAME, msg, strlen(msg));
    printf("write: %s\n", blusys_err_string(err));

    /* Exists + size */
    bool exists = false;
    blusys_fs_exists(fs, FILE_NAME, &exists);
    printf("exists: %s\n", exists ? "yes" : "no");

    size_t sz = 0;
    blusys_fs_size(fs, FILE_NAME, &sz);
    printf("size: %u bytes\n", (unsigned) sz);

    /* Read back */
    char buf[64] = {0};
    size_t n = 0;
    err = blusys_fs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("read (%u bytes): %s", (unsigned) n, buf);

    /* Append */
    const char *extra = "Appended line.\n";
    blusys_fs_append(fs, FILE_NAME, extra, strlen(extra));

    memset(buf, 0, sizeof(buf));
    blusys_fs_read(fs, FILE_NAME, buf, sizeof(buf) - 1, &n);
    printf("after append (%u bytes):\n%s", (unsigned) n, buf);

    /* Remove */
    err = blusys_fs_remove(fs, FILE_NAME);
    printf("remove: %s\n", blusys_err_string(err));

    blusys_fs_exists(fs, FILE_NAME, &exists);
    printf("exists after remove: %s\n", exists ? "yes" : "no");

    /* Unmount */
    blusys_fs_unmount(fs);
    printf("unmounted\n");
}
