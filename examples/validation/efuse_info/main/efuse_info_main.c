#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "blusys/blusys.h"

static bool read_bool(const char *name,
                      blusys_err_t (*fn)(bool *),
                      bool *out_value)
{
    blusys_err_t err = fn(out_value);
    if (err != BLUSYS_OK) {
        printf("%s error: %s\n", name, blusys_err_string(err));
        return false;
    }

    return true;
}

void app_main(void)
{
    uint8_t mac[BLUSYS_EFUSE_MAC_LEN];
    uint16_t chip_revision = 0;
    uint32_t package_version = 0;
    uint32_t secure_version = 0;
    bool flash_encryption_enabled = false;
    bool secure_boot_enabled = false;
    blusys_err_t err;

    printf("Blusys efuse info\n");
    printf("target: %s\n", blusys_target_name());
    printf("efuse supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_EFUSE) ? "yes" : "no");

    err = blusys_efuse_base_mac(mac);
    if (err != BLUSYS_OK) {
        printf("base_mac error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_efuse_chip_revision(&chip_revision);
    if (err != BLUSYS_OK) {
        printf("chip_revision error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_efuse_package_version(&package_version);
    if (err != BLUSYS_OK) {
        printf("package_version error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_efuse_secure_version(&secure_version);
    if (err != BLUSYS_OK) {
        printf("secure_version error: %s\n", blusys_err_string(err));
        return;
    }

    if (!read_bool("flash_encryption_enabled",
                   blusys_efuse_flash_encryption_enabled,
                   &flash_encryption_enabled)) {
        return;
    }

    if (!read_bool("secure_boot_enabled",
                   blusys_efuse_secure_boot_enabled,
                   &secure_boot_enabled)) {
        return;
    }

    printf("base_mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("chip_revision: %u\n", chip_revision);
    printf("package_version: %lu\n", (unsigned long)package_version);
    printf("secure_version: %lu\n", (unsigned long)secure_version);
    printf("flash_encryption_enabled: %s\n", flash_encryption_enabled ? "yes" : "no");
    printf("secure_boot_enabled: %s\n", secure_boot_enabled ? "yes" : "no");
}
