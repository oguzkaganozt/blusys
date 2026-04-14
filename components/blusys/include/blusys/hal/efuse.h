#ifndef BLUSYS_EFUSE_H
#define BLUSYS_EFUSE_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLUSYS_EFUSE_MAC_LEN 6

blusys_err_t blusys_efuse_base_mac(uint8_t out_mac[BLUSYS_EFUSE_MAC_LEN]);
blusys_err_t blusys_efuse_chip_revision(uint16_t *out_revision);
blusys_err_t blusys_efuse_package_version(uint32_t *out_version);
blusys_err_t blusys_efuse_secure_version(uint32_t *out_version);
blusys_err_t blusys_efuse_flash_encryption_enabled(bool *out_enabled);
blusys_err_t blusys_efuse_secure_boot_enabled(bool *out_enabled);

#ifdef __cplusplus
}
#endif

#endif
