#include "blusys/efuse.h"

#include <stdbool.h>
#include <stdint.h>

#include "blusys/internal/blusys_esp_err.h"

#include "esp_chip_info.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_flash_encrypt.h"
#include "esp_mac.h"

blusys_err_t blusys_efuse_base_mac(uint8_t out_mac[BLUSYS_EFUSE_MAC_LEN])
{
    if (out_mac == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    return blusys_translate_esp_err(esp_efuse_mac_get_default(out_mac));
}

blusys_err_t blusys_efuse_chip_revision(uint16_t *out_revision)
{
    esp_chip_info_t chip_info;

    if (out_revision == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_chip_info(&chip_info);
    *out_revision = chip_info.revision;

    return BLUSYS_OK;
}

blusys_err_t blusys_efuse_package_version(uint32_t *out_version)
{
    if (out_version == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_version = esp_efuse_get_pkg_ver();

    return BLUSYS_OK;
}

blusys_err_t blusys_efuse_secure_version(uint32_t *out_version)
{
    if (out_version == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_version = esp_efuse_read_secure_version();

    return BLUSYS_OK;
}

blusys_err_t blusys_efuse_flash_encryption_enabled(bool *out_enabled)
{
    if (out_enabled == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out_enabled = esp_flash_encryption_enabled();

    return BLUSYS_OK;
}

blusys_err_t blusys_efuse_secure_boot_enabled(bool *out_enabled)
{
    if (out_enabled == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

#if CONFIG_IDF_TARGET_ESP32
    *out_enabled = esp_efuse_read_field_bit(ESP_EFUSE_ABS_DONE_0) ||
                   esp_efuse_read_field_bit(ESP_EFUSE_ABS_DONE_1);
#else
    *out_enabled = esp_efuse_read_field_bit(ESP_EFUSE_SECURE_BOOT_EN);
#endif

    return BLUSYS_OK;
}
