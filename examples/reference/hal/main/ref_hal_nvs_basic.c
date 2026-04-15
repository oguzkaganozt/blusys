#include "sdkconfig.h"

#if CONFIG_REF_HAL_REFERENCE_SCENARIO_NVS_BASIC

#include <stdio.h>
#include <stdlib.h>

#include "blusys/blusys.h"

void run_nvs_basic(void)
{
    blusys_nvs_t *nvs;
    blusys_err_t  err;

    /* Open namespace in read-write mode */
    err = blusys_nvs_open("app_config", BLUSYS_NVS_READWRITE, &nvs);
    if (err != BLUSYS_OK) {
        printf("nvs_open failed: %s\n", blusys_err_string(err));
        return;
    }

    /* Write typed values */
    blusys_nvs_set_u8(nvs,  "boot_count",  42);
    blusys_nvs_set_u32(nvs, "device_id",   0xDEADBEEF);
    blusys_nvs_set_i32(nvs, "temperature", -15);
    blusys_nvs_set_str(nvs, "fw_tag",      "v3.0.0-rc1");

    /* Commit writes to flash before reading back */
    err = blusys_nvs_commit(nvs);
    if (err != BLUSYS_OK) {
        printf("commit failed: %s\n", blusys_err_string(err));
        blusys_nvs_close(nvs);
        return;
    }

    /* Read back integers */
    uint8_t  boot_count = 0;
    uint32_t device_id  = 0;
    int32_t  temp       = 0;

    blusys_nvs_get_u8(nvs,  "boot_count",  &boot_count);
    blusys_nvs_get_u32(nvs, "device_id",   &device_id);
    blusys_nvs_get_i32(nvs, "temperature", &temp);

    printf("boot_count : %u\n",          boot_count);
    printf("device_id  : 0x%08lX\n",     (unsigned long)device_id);
    printf("temperature: %ld C\n",       (long)temp);

    /* Two-call pattern for string: get length, allocate, then read */
    size_t len = 0;
    err = blusys_nvs_get_str(nvs, "fw_tag", NULL, &len);
    if (err == BLUSYS_OK) {
        char *tag = malloc(len);
        if (tag != NULL) {
            blusys_nvs_get_str(nvs, "fw_tag", tag, &len);
            printf("fw_tag     : %s\n", tag);
            free(tag);
        }
    }

    blusys_nvs_close(nvs);
    printf("Done.\n");
}


#else

void run_nvs_basic(void) {}

#endif /* CONFIG_REF_HAL_REFERENCE_SCENARIO_NVS_BASIC */
